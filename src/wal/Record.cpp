#include "Record.h"
#include <iostream>

namespace {
    void writeUint32(ostream& out, uint32_t value) {
        const char bytes[4] = {
            static_cast<char>((value >> 0) & 0xFF),
            static_cast<char>((value >> 8) & 0xFF),
            static_cast<char>((value >> 16) & 0xFF),
            static_cast<char>((value >> 24) & 0xFF)
        };

        out.write(bytes, 4);
    }

    void writeUint64(ostream& out, uint64_t value) {
        const char bytes[8] = {
            static_cast<char>((value >> 0) & 0xFF),
            static_cast<char>((value >> 8) & 0xFF),
            static_cast<char>((value >> 16) & 0xFF),
            static_cast<char>((value >> 24) & 0xFF),
            static_cast<char>((value >> 32) & 0xFF),
            static_cast<char>((value >> 40) & 0xFF),
            static_cast<char>((value >> 48) & 0xFF),
            static_cast<char>((value >> 56) & 0xFF)
        };

        out.write(bytes, 8);
    }

    void writeMagic(ostream& out) {
        uint32_t magic = 0x314C4157; // WAL1 - litte endian
        writeUint32(out, magic);
    }

    void writePayloadLen(ostream& out, const vector<uint8_t>& payload) {
        uint32_t payloadLen = payload.size();
        writeUint32(out, payloadLen);
    }

    void writeLsn(ostream& out, Lsn lsn) {
        writeUint64(out, lsn);
    }

    void writeCrc(ostream& out) {
        // TODO - this is a temp CRC for now
        uint32_t crc = 0x00000000;
        writeUint32(out, crc);
    }

    void writePayload(ostream& out, const vector<uint8_t>& payload) {
        out.write(reinterpret_cast<const char*>(payload.data()), payload.size());
    }

    uint32_t readUint32(istream& in, uint64_t offset) {
        in.seekg(static_cast<std::streamoff>(offset), std::ios::beg);

        unsigned char bytes[4];

        in.read(reinterpret_cast<char*>(bytes), 4);

        if (!in) {
            throw std::runtime_error("failed to read uint32");
        }

        return (static_cast<uint32_t>(bytes[0]) << 0)  |
            (static_cast<uint32_t>(bytes[1]) << 8)  |
            (static_cast<uint32_t>(bytes[2]) << 16) |
            (static_cast<uint32_t>(bytes[3]) << 24);
    }

    uint64_t readUint64(istream& in, uint64_t offset) {
        in.seekg(static_cast<std::streamoff>(offset), std::ios::beg);

        unsigned char bytes[8];

        in.read(reinterpret_cast<char*>(bytes), 8);

        if (!in) {
            throw std::runtime_error("failed to read uint64");
        }

        return (static_cast<uint64_t>(bytes[0]) << 0)  |
            (static_cast<uint64_t>(bytes[1]) << 8)  |
            (static_cast<uint64_t>(bytes[2]) << 16) |
            (static_cast<uint64_t>(bytes[3]) << 24) |
            (static_cast<uint64_t>(bytes[4]) << 32) |
            (static_cast<uint64_t>(bytes[5]) << 40) |
            (static_cast<uint64_t>(bytes[6]) << 48) |
            (static_cast<uint64_t>(bytes[7]) << 56);
    }
    
    uint32_t readMagic(istream& in, uint64_t offset) {
        return readUint32(in, offset);
    }

    uint32_t readPayloadLen(istream& in, uint64_t offset){
        return readUint32(in, offset);
    }

    Lsn readLsn(istream& in, uint64_t offset) {
        return readUint64(in, offset);
    }

    uint32_t readCrc(istream& in, uint64_t offset){
        return readUint32(in, offset);
    }

    const vector<uint8_t> readPayload(istream& in, uint64_t offset, const uint32_t payload_len) {
        in.seekg(static_cast<std::streamoff>(offset), std::ios::beg);

        vector<uint8_t> bytes(payload_len);
        in.read(reinterpret_cast<char*> (bytes.data()), payload_len);

        return bytes;
    }
}

void writeRecord(ostream& out, Lsn lsn, const vector<uint8_t>& payload) {
    std::cout << "Writing record" << std::endl;

    // TODO - need better error handling for this case, but this at least works for now
    if (payload.size() > UINT32_MAX)
        return;

    // Need to write the record data to the input file.
    // Write the data in-order per design and make sure to handle the endianess
    writeMagic(out);
    writePayloadLen(out, payload);
    writeLsn(out, lsn);
    writeCrc(out);
    writePayload(out, payload);
}

RecordReadResult readRecord(istream& in, uint64_t nextOffset) {
    RecordReadResult res;
    uint64_t offset = nextOffset;

    if (readMagic(in, offset) != 0x314C4157) {
        res.status = RecordReadStatus::Corrupt;
        return res;
    }
    offset += SIZE_MAGIC;

    uint32_t payloadLen = readPayloadLen(in, offset);
    offset += SIZE_PAYLOAD_LEN;

    res.lsn = readLsn(in, offset);
    offset += SIZE_LSN;

    uint32_t crc = readCrc(in, offset);
    offset += SIZE_CRC;

    res.payload = readPayload(in, offset, payloadLen);
    offset += res.payload.size();

    res.nextOffset = offset;

    res.status = RecordReadStatus::Ok;

    return res;
}