#include "Wal.h"
#include <iostream>

using namespace std;

int main()
{
    Wal wal{"data"};
    wal.recover();

    const auto lsn = wal.append({'h', 'e', 'l', 'l', 'o'});
    cout << "Appended record at LSN " << lsn << '\n';

    const auto lsn2 = wal.append({'g', 'o', 'o', 'd', 'b', 'y', 'e'});
    cout << "Appended record at LSN " << lsn2 << '\n';

    vector<uint8_t> record = wal.read(lsn);

    cout << "Data from read LSN " << lsn << ": ";
    for (auto byte : record) {
        cout << byte;
    }
    cout << "\n";

    vector<uint8_t> record2 = wal.read(lsn2);

    cout << "Data from read LSN " << lsn2 << ": ";
    for (auto byte : record2) {
        cout << byte;
    }
    cout << "\n";
}