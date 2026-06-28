#pragma once

#include "WalWriter.h"
#include "Record.h"

#include <filesystem>
#include <WinSock2.h>
#include <cstddef>
#include <cstdint>
#include <atomic>
#include <vector>
#include <thread>

class WalServer {
public:
    explicit WalServer(std::filesystem::path dataDir, int port);
    ~WalServer();

    WalServer(const WalServer&) = delete;
    WalServer& operator=(const WalServer&) = delete;

    void run();
    void start(int port);
    void stop();
    void join();

private:
    static constexpr std::size_t MAX_PAYLOAD_LEN = 1024;

    void handleClient(SOCKET clientSocket);
    bool readExact(SOCKET clientSocket, void* buffer, size_t bytesToRead);
    void sendExact(SOCKET clientSocket, const void* buffer, size_t bytesToSend);
    void serializeLsn(Lsn lsn, uint8_t buffer[8]);

    WalWriter m_writer;
    SOCKET m_listenSocket;
    std::atomic<bool> m_shutdown;
    std::vector<std::thread> m_handlerThreads;
};