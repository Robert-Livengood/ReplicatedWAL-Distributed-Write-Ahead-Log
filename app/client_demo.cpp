#include "Client.h"
#include "Record.h"

#include <vector>
#include <cstdint>
#include <iostream>
#include <thread>
#include <string>
#include <exception>

using namespace std;

void runClientThread(string host, int port, int clientId) {
    try {
        Client client;

        string message = "Hello from client " + to_string(clientId);
        vector<uint8_t> payload(message.begin(), message.end());

        Lsn lsn = client.runOnce(host, port, payload);

        cout << "\nClient " << clientId << " received LSN: " << lsn << "\n";
    }
    catch (const exception ex) {
        cerr << "Client " << clientId << " failed: " << ex.what() << "\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "usage: wal_client_demo <host> <port> <numClients>\n";
        return 1;
    }

    // default host = "127.0.0.1"
    string host = argv[1];
    int port = stoi(argv[2]);
    uint16_t numClients = stoi(argv[3]);

    vector<thread> threads;

    for (int i = 0; i < numClients; i++) {
        threads.emplace_back(thread(runClientThread, host, port, i));
    }

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    return 0;
}