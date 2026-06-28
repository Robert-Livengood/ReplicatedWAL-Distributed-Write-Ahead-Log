// #include "WalWriter.h"
#include "WalServer.h"

#include <iostream>
#include <string>
#include <exception>
#include <thread>

using namespace std;

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "usage: wal_server_demo <data-dir> <port>\n";
        return 1;
    }
    
    string dataDir(argv[1]);
    int port = stoi(argv[2]);

    try {
        WalServer server{dataDir, port};
        exception_ptr serverException = nullptr;

        thread serverThread([&server, &serverException]() {
            try {
                server.run();
            }
            catch (...) {
                serverException = current_exception();
            }
        });

        cout << "WAL server running on port " << port << "\n";
        cout << "Press Enter to shut down...\n";

        string line;
        getline(cin, line);

        server.stop();

        if (serverThread.joinable()) {
            serverThread.join();
        }

        server.join();

        if (serverException) {
            rethrow_exception(serverException);
        }
    }
    catch (const exception& ex) {
        cerr << "server failed: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}