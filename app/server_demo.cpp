// #include "WalWriter.h"
#include "WalServer.h"

#include <iostream>
#include <string>

using namespace std;

int main(int argc, char* argv[]) {
    string dataDir(argv[0]);
    int port = stoi(argv[1]);
    
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