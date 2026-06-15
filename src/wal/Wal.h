#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

using Lsn = std::uint64_t;
using std::vector;
using namespace std;

namespace fs = std::filesystem;

class Wal {
public:
    explicit Wal(fs::path dataDir);

    Lsn append(const vector<uint8_t>& payload);
    
    vector<uint8_t> read(Lsn lsn) const;

    void recover();

private:
    fs::path dataDir_;
    Lsn nextLsn_;
};