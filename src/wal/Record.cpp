#include "Record.h"
#include <iostream>

namespace {
    void serialize_uint32(uint32_t value, uint8_t bytes[4]) {
        bytes[0] = static_cast<uint8_t>((value >> 0) & 0xFF);
        bytes[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
        bytes[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
        bytes[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
    }

    void serialize_uint64(uint64_t value, uint8_t bytes[8]) {
        bytes[0] = static_cast<uint8_t>((value >> 0) & 0xFF);
        bytes[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
        bytes[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
        bytes[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
        bytes[4] = static_cast<uint8_t>((value >> 32) & 0xFF);
        bytes[5] = static_cast<uint8_t>((value >> 40) & 0xFF);
        bytes[6] = static_cast<uint8_t>((value >> 48) & 0xFF);
        bytes[7] = static_cast<uint8_t>((value >> 56) & 0xFF);
    }

    void write_uint32(ostream& out, uint8_t bytes[4]) {
        out.write(reinterpret_cast<const char*>(bytes), 4);
    }

    void write_uint64(ostream& out, uint8_t bytes[8]) {
        out.write(reinterpret_cast<const char*>(bytes), 8);
    }

    uint32_t initCRC() {
        // Standard CRC32 starts with all bits set.
        return 0xFFFFFFFF;
    }

    uint32_t finalizeCRC(uint32_t crc) {
        // Standard CRC32 inverts the bits after all input bytes are processed.
        return crc ^ 0xFFFFFFFF;
    }

    uint32_t updateCRC(uint32_t crc, const uint8_t* data, size_t len) {
        // Standard CRC32 uses the reflected polynomial 0xEDB88320.
        // This function updates the running CRC state with the provided bytes.
        // The caller is responsible for initializing the CRC before the first update
        // and finalizing it after all bytes have been processed.
        static const uint32_t POLY = 0xEDB88320;

        for (size_t i = 0; i < len; ++i) {
            // Mix the next input byte into the low byte of the running CRC.
            crc ^= data[i];

            // Process each of the 8 bits in the current byte.
            for (int bit = 0; bit < 8; ++bit) {
                // If the low bit is set, shift and apply the CRC polynomial.
                // Otherwise, only shift. This is the bit-by-bit CRC division step.
                if ((crc & 1) != 0) {
                    crc = (crc >> 1) ^ POLY;
                } else {
                    crc = crc >> 1;
                }
            }
        }

        return crc;
    }

    void writeAndUpdateCRC_uint32(ostream& out, uint32_t& crc, uint32_t value) {
        uint8_t bytes[4];
        serialize_uint32(value, bytes);

        crc = updateCRC(crc, bytes, 4);
        write_uint32(out, bytes);
    }

    void writeAndUpdateCRC_uint64(ostream& out, uint32_t& crc, uint64_t value) {
        uint8_t bytes[8];
        serialize_uint64(value, bytes);

        crc = updateCRC(crc, bytes, 8);
        write_uint64(out, bytes);
    }

    void writePayload(ostream& out, const vector<uint8_t>& payload) {
        out.write(reinterpret_cast<const char*>(payload.data()), payload.size());
    }

    uint32_t readUint32(istream& in, uint64_t offset, uint32_t& crc) {
        in.seekg(static_cast<std::streamoff>(offset), std::ios::beg);

        unsigned char bytes[4];

        in.read(reinterpret_cast<char*>(bytes), 4);

        if (!in) {
            throw std::runtime_error("failed to read uint32");
        }

        crc = updateCRC(crc, bytes, 4);

        return (static_cast<uint32_t>(bytes[0]) << 0)  |
            (static_cast<uint32_t>(bytes[1]) << 8)  |
            (static_cast<uint32_t>(bytes[2]) << 16) |
            (static_cast<uint32_t>(bytes[3]) << 24);
    }

    uint64_t readUint64(istream& in, uint64_t offset, uint32_t& crc) {
        in.seekg(static_cast<std::streamoff>(offset), std::ios::beg);

        unsigned char bytes[8];

        in.read(reinterpret_cast<char*>(bytes), 8);

        if (!in) {
            throw std::runtime_error("failed to read uint64");
        }

        crc = updateCRC(crc, bytes, 8);

        return (static_cast<uint64_t>(bytes[0]) << 0)  |
            (static_cast<uint64_t>(bytes[1]) << 8)  |
            (static_cast<uint64_t>(bytes[2]) << 16) |
            (static_cast<uint64_t>(bytes[3]) << 24) |
            (static_cast<uint64_t>(bytes[4]) << 32) |
            (static_cast<uint64_t>(bytes[5]) << 40) |
            (static_cast<uint64_t>(bytes[6]) << 48) |
            (static_cast<uint64_t>(bytes[7]) << 56);
    }
    
    uint32_t readMagic(istream& in, uint64_t offset, uint32_t& crc) {
        return readUint32(in, offset, crc);
    }

    uint32_t readPayloadLen(istream& in, uint64_t offset, uint32_t& crc){
        return readUint32(in, offset, crc);
    }

    Lsn readLsn(istream& in, uint64_t offset, uint32_t& crc) {
        return readUint64(in, offset, crc);
    }

    uint32_t readCrc(istream& in, uint64_t offset){
        uint32_t throwaway = initCRC();
        return readUint32(in, offset, throwaway);
    }

    const vector<uint8_t> readPayload(istream& in, uint64_t offset, const uint32_t payload_len, uint32_t& crc) {
        in.seekg(static_cast<std::streamoff>(offset), std::ios::beg);

        vector<uint8_t> bytes(payload_len);
        in.read(reinterpret_cast<char*> (bytes.data()), payload_len);

        if (!in) {
            throw std::runtime_error("failed to read payload");
        }

        const uint8_t* tempBuffer = bytes.data();
        crc = updateCRC(crc, tempBuffer, payload_len);

        return bytes;
    }
}

void writeRecord(ostream& out, Lsn lsn, const vector<uint8_t>& payload) {
    std::cout << "Writing record" << std::endl;

    // TODO - need better error handling for this case, but this at least works for now
    if (payload.size() > UINT32_MAX)
        return;

    uint32_t magic = 0x314C4157; // WAL1 - litte endian
    uint32_t payloadLen = static_cast<uint32_t>(payload.size());

    // initialize the CRC and then update after each write step.
    // CRC covers magic + payload length + LSN + payload.
    // The stored CRC field itself is excluded.
    uint32_t crc = initCRC();

    // Need to write the record data to the input file.
    // Write the data in-order per design and make sure to handle the endianess
    writeAndUpdateCRC_uint32(out, crc, magic);
    writeAndUpdateCRC_uint32(out, crc, payloadLen);
    writeAndUpdateCRC_uint64(out, crc, lsn);

    if (!payload.empty()) {
        // Payload is included in the CRC even though it is written after the CRC field.
        crc = updateCRC(crc, payload.data(), payloadLen);
    }

    // finalize the CRC
    const uint32_t finalCRC = finalizeCRC(crc);

    // serialize and write the CRC
    uint8_t bytesCRC[4]; 
    serialize_uint32(finalCRC, bytesCRC);
    write_uint32(out, bytesCRC);

    // payload is the final thing to write
    writePayload(out, payload);
}

RecordReadResult readRecord(istream& in, uint64_t nextOffset) {
    RecordReadResult res;
    uint64_t offset = nextOffset;

    // CRC covers magic + payload length + LSN + payload.
    // The stored CRC field itself is excluded.
    uint32_t crc = initCRC();

    if (readMagic(in, offset, crc) != 0x314C4157) {
        res.status = RecordReadStatus::Corrupt;
        return res;
    }
    offset += SIZE_MAGIC;

    uint32_t payloadLen = readPayloadLen(in, offset, crc);
    offset += SIZE_PAYLOAD_LEN;

    res.lsn = readLsn(in, offset, crc);
    offset += SIZE_LSN;

    uint32_t dataCRC = readCrc(in, offset);
    offset += SIZE_CRC;

    res.payload = readPayload(in, offset, payloadLen, crc);
    offset += res.payload.size();

    res.nextOffset = offset;

    const uint32_t computedCRC = finalizeCRC(crc);

    if (dataCRC != computedCRC) {
        res.status = RecordReadStatus::Corrupt;
    }
    else {
        res.status = RecordReadStatus::Ok;
    }

    return res;
}