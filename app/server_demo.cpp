// #include "WalWriter.h"
#include "WalServer.h"

#include <iostream>
#include <string>

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
        server.run();
    }
    catch (const exception& ex) {
        cerr << "server failed: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}