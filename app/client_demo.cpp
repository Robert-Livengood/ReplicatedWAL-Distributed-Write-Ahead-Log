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
    // default host = "127.0.0.1"
    string host = argv[0];
    int port = stoi(argv[1]);
    uint16_t numClients = stoi(argv[2]);

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