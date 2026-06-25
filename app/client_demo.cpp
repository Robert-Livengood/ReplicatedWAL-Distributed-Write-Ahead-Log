#include "Client.h"
#include "Record.h"

#include <vector>
#include <cstdint>
#include <iostream>

using namespace std;

int main() {
    Client client;
    vector<uint8_t> payload = {'H','e','l','l','o'};
    Lsn lsn = client.runOnce("127.0.0.1", 8080, payload);
    cout << "LSN: " << lsn << "\n";
}