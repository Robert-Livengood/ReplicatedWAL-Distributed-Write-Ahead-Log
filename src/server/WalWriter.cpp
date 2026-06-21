#include "WalWriter.h"
#include "Wal.h"

#include <filesystem>
#include <mutex>

using namespace std;

WalWriter::WalWriter(const filesystem::path dataDir) : m_wal(move(dataDir)) {
    m_wal.recover();
    m_thread = thread(&WalWriter::writerLoop, this);
}

WalWriter::~WalWriter() {
    {
        lock_guard<mutex> lock(m_mutex);
        m_stopping = true;
    }

    m_cv.notify_one();

    if (m_thread.joinable()) {
        m_thread.join();
    }
}

Lsn WalWriter::append(const vector<uint8_t>& payload) {
    auto request = make_unique<AppendRequest>();
    request->payload = payload;

    future<Lsn> future = request->result.get_future();

    {
        lock_guard<mutex> lock(m_mutex);
        m_queue.push(move(request));
    }

    m_cv.notify_one();

    return future.get();
}

void WalWriter::writerLoop() {
    while (true) {
        unique_ptr<AppendRequest> request;

        {
            unique_lock<mutex> lock(m_mutex);

            m_cv.wait(lock, [this] {
                return m_stopping || !m_queue.empty();
            });
        

            if (m_stopping && m_queue.empty()) {
                return;
            }

            request = move(m_queue.front());
            m_queue.pop();
        }

        try {
            Lsn lsn = m_wal.append(request->payload);
            request->result.set_value(lsn);
        }
        catch (...) {
            request->result.set_exception(current_exception());
        }
    }
}