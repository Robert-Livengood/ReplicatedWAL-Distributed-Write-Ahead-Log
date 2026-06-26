#pragma once

#include "WalWriter.h"
#include "Record.h"

#include <filesystem>
#include <WinSock2.h>
#include <cstddef>
#include <cstdint>

class WalServer {
public:
    explicit WalServer(std::filesystem::path dataDir);
    ~WalServer();

    WalServer(const WalServer&) = delete;
    WalServer& operator=(const WalServer&) = delete;

    void run();

private:
    static constexpr int PORT = 8080;
    static constexpr std::size_t MAX_PAYLOAD_LEN = 1024;

    void handleClient(SOCKET clientSocket);
    void readExact(SOCKET clientSocket, void* buffer, size_t bytesToRead);
    void sendExact(SOCKET clientSocket, const void* buffer, size_t bytesToSend);
    void serializeLsn(Lsn lsn, uint8_t buffer[8]);

    WalWriter m_writer;
    SOCKET m_listenSocket;
};