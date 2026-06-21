#pragma once

#include "Wal.h"

#include <filesystem>
#include <future>
#include <mutex>
#include <queue>

class WalWriter {
public:
    explicit WalWriter(std::filesystem::path dataDir);
    ~WalWriter();

    WalWriter(const WalWriter&) = delete;
    WalWriter& operator=(const WalWriter&) = delete;

    Lsn append(const std::vector<uint8_t>& payload);

private:
    struct AppendRequest {
        std::vector<uint8_t> payload;
        std::promise<Lsn> result;
    };

    void writerLoop();

    Wal m_wal;

    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::queue<std::unique_ptr<AppendRequest>> m_queue;

    bool m_stopping = false;
};