#include "Client.h"
#include "Record.h"

#include <vector>
#include <cstdint>
#include <iostream>
#include <thread>
#include <string>
#include <exception>

using namespace std;

void runClientThread(int clientId) {
    try {
        Client client;

        string message = "Hello from client " + to_string(clientId);
        vector<uint8_t> payload(message.begin(), message.end());

        Lsn lsn = client.runOnce("127.0.0.1", 8080, payload);

        cout << "\nClient " << clientId << " received LSN: " << lsn << "\n";
    }
    catch (const exception ex) {
        cerr << "Client " << clientId << " failed: " << ex.what() << "\n";
    }
}

int main() {
    vector<thread> threads;

    for (int i = 0; i < 10; i++) {
        threads.emplace_back(thread(runClientThread, i));
    }

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    return 0;
}