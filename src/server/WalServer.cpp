#include "WalServer.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <filesystem>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <cstdint>
#include <thread>

WalServer::WalServer(std::filesystem::path dataDir, int port)
    : m_writer(std::move(dataDir)),
      m_listenSocket(INVALID_SOCKET),
      m_shutdown(false),
      m_handlerThreads{}
{
    start(port);
}

WalServer::~WalServer() {
    if (m_listenSocket != INVALID_SOCKET) {
        closesocket(m_listenSocket);
    }

    WSACleanup();
}

void WalServer::run() {
    // only run until shutdown signal is received
    while (!m_shutdown.load()) {
        sockaddr_in clientAddr{};
        int clientAddrLen = sizeof(clientAddr);

        SOCKET clientSocket = accept(
            m_listenSocket,
            reinterpret_cast<sockaddr*>(&clientAddr),
            &clientAddrLen
        );

        if (clientSocket == INVALID_SOCKET) {
            if (m_shutdown.load()) {
                break;
            }

            throw std::runtime_error("socket accept failed");
        }

        if (m_shutdown.load()) {
            closesocket(clientSocket);
            continue;
        }

        m_handlerThreads.emplace_back(&WalServer::handleClient, this, clientSocket);
    }
}

void WalServer::start(int port) {
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed with error: " << WSAGetLastError() << "\n";
        throw std::runtime_error("WSAStartup failed");
    }

    m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (m_listenSocket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed with error: " << WSAGetLastError() << "\n";
        WSACleanup();
        throw std::runtime_error("socket creation failed");
    }

    sockaddr_in service{};
    service.sin_family = AF_INET;
    service.sin_port = htons(port);
    service.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(m_listenSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
        std::cerr << "Socket bind failed with error: " << WSAGetLastError() << "\n";
        closesocket(m_listenSocket);
        WSACleanup();
        throw std::runtime_error("socket bind failed");
    }

    if (listen(m_listenSocket, 5) == SOCKET_ERROR) {
        std::cerr << "Socket listen failed with error: " <<WSAGetLastError() << "\n";
        closesocket(m_listenSocket);
        WSACleanup();
        throw std::runtime_error("socket listen failed");
    }
}

void WalServer::stop() {
    // set shutdown
    m_shutdown.store(true);
    // close listen socket
    if (m_listenSocket != INVALID_SOCKET) {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }
}

void WalServer::join() {
    // join handler threads
    for (std::thread& t : m_handlerThreads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void WalServer::handleClient(SOCKET clientSocket) {
    try {
        // receive the payload from the client
        uint8_t payloadLenBytes[4];
        if (!readExact(clientSocket, &payloadLenBytes, sizeof(payloadLenBytes))) {
            // client connected and closed connection before sending anything
            // This can happen in readiness probes
            return;
        }

        uint32_t payloadLen = (static_cast<uint32_t>(payloadLenBytes[0]) << 0)  |
                              (static_cast<uint32_t>(payloadLenBytes[1]) << 8)  |
                              (static_cast<uint32_t>(payloadLenBytes[2]) << 16) |
                              (static_cast<uint32_t>(payloadLenBytes[3]) << 24);

        // if the payload length is too large -> reject it
        if (payloadLen > MAX_PAYLOAD_LEN) {
            throw std::runtime_error("Client payload too large");
        }

        std::vector<uint8_t> payload(payloadLen);
        if (!readExact(clientSocket, payload.data(), payload.size())) {
            throw std::runtime_error("client disconnected before sending complete payload");
        }

        // queue the payload to the writer
        Lsn lsn = m_writer.append(payload);

        // send the LSN back to the client
        uint8_t lsnBuffer[8];
        serializeLsn(lsn, lsnBuffer);
        sendExact(clientSocket, lsnBuffer, sizeof(lsnBuffer));
    }
    catch (const std::exception ex) {
        std::cerr << "Client handler failed: " << ex.what() << "\n";
    }

    closesocket(clientSocket);
}

bool WalServer::readExact(SOCKET clientSocket, void* buffer, size_t bytesToRead) {
    size_t totalRead = 0;
    char* in = static_cast<char*> (buffer);
    while (totalRead < bytesToRead) {
        int rcvd = recv(clientSocket, in + totalRead, static_cast<int>(bytesToRead - totalRead), 0);
        if (rcvd == 0) {
            // client connection closed -> not necessarily an error i.e. readiness probe
            return false;
        }
        else if (rcvd == SOCKET_ERROR) {
            // error in recv
            std::cerr << "Socket recv failed with error: " << WSAGetLastError() << "\n";
            throw std::runtime_error("recv from client failed");
        }
        totalRead += rcvd;
    }
    return true;
}

void WalServer::sendExact(SOCKET clientSocket, const void* buffer, size_t bytesToSend) {
    size_t totalSent = 0;
    const char* out = static_cast<const char*>(buffer);
    while (totalSent < bytesToSend) {
        int sent = send(clientSocket, out + totalSent, static_cast<int>(bytesToSend - totalSent), 0);
        if (sent == 0) {
            // client connection closed
            throw std::runtime_error("send to client sent no bytes");
        }
        else if (sent == SOCKET_ERROR) {
            // error in send
            std::cerr << "Socket send failed with error: " << WSAGetLastError() << "\n";
            throw std::runtime_error("send to client failed");
        }
        totalSent += sent;
    }
}

void WalServer::serializeLsn(Lsn lsn, uint8_t buffer[8]) {
    // little endian to match Record.cpp endianess
    buffer[0] = static_cast<uint8_t>((lsn >> 0) & 0xFF);
    buffer[1] = static_cast<uint8_t>((lsn >> 8) & 0xFF);
    buffer[2] = static_cast<uint8_t>((lsn >> 16) & 0xFF);
    buffer[3] = static_cast<uint8_t>((lsn >> 24) & 0xFF);
    buffer[4] = static_cast<uint8_t>((lsn >> 32) & 0xFF);
    buffer[5] = static_cast<uint8_t>((lsn >> 40) & 0xFF);
    buffer[6] = static_cast<uint8_t>((lsn >> 48) & 0xFF);
    buffer[7] = static_cast<uint8_t>((lsn >> 56) & 0xFF);
}