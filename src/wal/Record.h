#pragma once

#include <cstdint>
#include <iosfwd>
#include <vector>

using Lsn = std::uint64_t;
using std::ostream;
using std::vector;
using std::istream;

constexpr uint8_t SIZE_MAGIC = 4;
constexpr uint8_t SIZE_PAYLOAD_LEN = 4;
constexpr uint8_t SIZE_LSN = 8;
constexpr uint8_t SIZE_CRC = 4;
constexpr uint8_t SIZE_HEADER = 20;

enum class RecordReadStatus {
    Ok,
    EndOfFile,
    Corrupt
};

struct RecordReadResult {
    RecordReadStatus status = RecordReadStatus::Ok;
    Lsn lsn = 0;
    vector<uint8_t> payload;
    uint64_t nextOffset = 0;
};

void writeRecord(ostream& out, Lsn lsn, const vector<uint8_t>& payload);
RecordReadResult readRecord(istream& in, uint64_t nextOffset, uintmax_t filesize);