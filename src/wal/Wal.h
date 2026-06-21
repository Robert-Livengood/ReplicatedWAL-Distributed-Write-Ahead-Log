#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>
#include <unordered_map>

using Lsn = std::uint64_t;

class Wal {
public:
    explicit Wal(std::filesystem::path dataDir);

    Lsn append(const std::vector<uint8_t>& payload);
    
    std::vector<uint8_t> read(Lsn lsn) const;

    void recover();

private:
    struct RecordLocation {
        std::filesystem::path segmentPath;
        uint64_t offset;
        uint64_t recordSize;
    };

    std::unordered_map<Lsn, RecordLocation> lsnMap;
    std::filesystem::path dataDir_;
    Lsn nextLsn_;
};