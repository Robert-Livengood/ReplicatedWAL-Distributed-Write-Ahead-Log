// #include "WalWriter.h"
#include "WalServer.h"

#include <iostream>

using namespace std;

int main() {
    try {
        WalServer server{"data"};
        server.run();
    }
    catch (const exception& ex) {
        cerr << "server failed: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}