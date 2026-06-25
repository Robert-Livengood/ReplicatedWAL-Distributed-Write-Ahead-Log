#pragma once

#include "Record.h"

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class Client {
public:
    Lsn runOnce(const std::string& host, uint16_t port, const std::vector<uint8_t>& payload);

private:
    static constexpr std::size_t MAX_PAYLOAD_LEN = 1024;

    void readExact(SOCKET serverSocket, void* buffer, size_t bytesToRead);
    void sendExact(SOCKET serverSocket, const void* buffer, size_t bytesToSend);
    Lsn decodeLsn(const uint8_t buffer[8]);
};