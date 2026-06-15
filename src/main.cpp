#include "Wal.h"
#include <iostream>

int main()
{
    Wal wal{"data"};
    wal.recover();

    const auto lsn = wal.append({'h', 'e', 'l', 'l', 'o'});

    std::cout << "Appended record at LSN " << lsn << '\n';
}