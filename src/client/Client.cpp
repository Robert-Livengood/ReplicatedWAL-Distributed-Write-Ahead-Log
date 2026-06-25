#include "Client.h"
#include "Record.h"

#include <WinSock2.h>
#include <ws2tcpip.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>

using namespace std;

Lsn Client::runOnce(const string& host, uint16_t port, const vector<uint8_t>& payload) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed with error: " << WSAGetLastError() << "\n";
        throw std::runtime_error("WSAStartup failed");
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed with error: " << WSAGetLastError() << "\n";
        WSACleanup();
        throw std::runtime_error("socket creation failed");
    }

    sockaddr_in service{};
    service.sin_family = AF_INET;
    service.sin_port = htons(port);
    service.sin_addr.s_addr = inet_addr(host.c_str());

    if (connect(serverSocket, reinterpret_cast<SOCKADDR*>(&service), sizeof(service)) == SOCKET_ERROR) {
        std::cerr << "Socket connect failed with error: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        WSACleanup();
        throw std::runtime_error("socket conenct failed");
    }

    // send the payload length first
    uint8_t payloadLenBuffer[4];
    uint32_t payloadLen = payload.size();
    if (payload.size() > UINT32_MAX) {
        throw runtime_error("payload too large");
    }
    // serialize the payloadLen -> little endian
    payloadLenBuffer[0] = static_cast<uint8_t>((payloadLen >> 0) & 0xFF);
    payloadLenBuffer[1] = static_cast<uint8_t>((payloadLen >> 8) & 0xFF);
    payloadLenBuffer[2] = static_cast<uint8_t>((payloadLen >> 16) & 0xFF);
    payloadLenBuffer[3] = static_cast<uint8_t>((payloadLen >> 24) & 0xFF);
    sendExact(serverSocket, payloadLenBuffer, sizeof(payloadLenBuffer));

    // now send the payload
    sendExact(serverSocket, payload.data(), payloadLen);

    if (shutdown(serverSocket, SD_SEND) == SOCKET_ERROR) {
        std::cerr << "shutdown failed with error: " << WSAGetLastError() << '\n';
    }

    // receive the LSN
    uint8_t lsnBuffer[8];
    readExact(serverSocket, lsnBuffer, sizeof(lsnBuffer));

    Lsn lsn = decodeLsn(lsnBuffer);

    
    closesocket(serverSocket);
    WSACleanup();

    return lsn;
}

void Client::readExact(SOCKET serverSocket, void* buffer, size_t bytesToRead) {
    size_t totalRead = 0;
    char* in = static_cast<char*> (buffer);
    while (totalRead < bytesToRead) {
        int rcvd = recv(serverSocket, in + totalRead, static_cast<int> (bytesToRead - totalRead), 0);
        if (rcvd == 0) {
            // server connection closed
            throw runtime_error("recv from server failed: connection closed");
        }
        else if (rcvd == SOCKET_ERROR) {
            // error in recieve
            cerr << "Socket recv failed with error: " << WSAGetLastError() << "\n";
            throw runtime_error("recv from server failed");
        }
        totalRead += rcvd;
    }
}

void Client::sendExact(SOCKET serverSocket, const void* buffer, size_t bytesToSend) {
    size_t totalSent = 0;
    const char* out = static_cast<const char*>(buffer);
    while (totalSent < bytesToSend) {
        int sent = send(serverSocket, out + totalSent, static_cast<int>(bytesToSend - totalSent), 0);
        if (sent == 0) {
            // server connection refused
            throw runtime_error("send to server sent no bytes");
        }
        else if (sent == SOCKET_ERROR) {
            // error in send
            cerr << "Socket send failed with error: " << WSAGetLastError() << "\n";
            throw runtime_error("Send to server failed");
        }
        totalSent += sent;
    }
}

Lsn Client::decodeLsn(const uint8_t buffer[8]) {
    // little endian to match Record.cpp endianess
    return (static_cast<uint64_t>(buffer[0]) << 0)  |
           (static_cast<uint64_t>(buffer[1]) << 8)  |
           (static_cast<uint64_t>(buffer[2]) << 16) |
           (static_cast<uint64_t>(buffer[3]) << 24) |
           (static_cast<uint64_t>(buffer[4]) << 32) |
           (static_cast<uint64_t>(buffer[5]) << 40) |
           (static_cast<uint64_t>(buffer[6]) << 48) |
           (static_cast<uint64_t>(buffer[7]) << 56);
}