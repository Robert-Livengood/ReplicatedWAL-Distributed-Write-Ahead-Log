#include "WalWriter.h"

#include <iostream>

using namespace std;

int main() {
    WalWriter writer{"data"};

    std::vector<uint8_t> payload = {'h', 'e', 'l', 'l', 'o'};

    Lsn lsn = writer.append(payload);

    std::cout << "Appended LSN: " << lsn << "\n";

    std::vector<std::thread> threads;
    std::vector<Lsn> lsns;
    std::mutex lsnsMutex;

    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&writer, &lsns, &lsnsMutex, i] {
            std::vector<uint8_t> payload = {static_cast<uint8_t>('a' + i)};
            Lsn lsn = writer.append(payload);

            {
                std::lock_guard<std::mutex> lock(lsnsMutex);
                lsns.push_back(lsn);
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    std::sort(lsns.begin(), lsns.end());

    for (Lsn lsn : lsns) {
        std::cout << "Returned LSN: " << lsn << "\n";
    }

    return 0;
}