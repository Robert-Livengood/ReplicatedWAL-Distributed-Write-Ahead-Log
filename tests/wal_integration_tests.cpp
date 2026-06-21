#include "Wal.h"
#include "Record.h"

#include <iostream>
#include <filesystem>
#include <string>
#include <fstream>

using namespace std;
namespace fs = std::filesystem;

bool test_cleanRestart() {
    const fs::path testDir = "test-data/clean-restart";
    if (fs::is_directory(testDir)) {
        fs::remove_all(testDir);
    }

    // store LSNs and their payloads to validate after recovery
    unordered_map<Lsn, vector<uint8_t>> testData;

    // using block to force the wal object to go out of scope
    {
        Wal wal{testDir};
        wal.recover();

        const vector<uint8_t> payload0 = {'h', 'e', 'l', 'l', 'o'};
        const auto lsn0 = wal.append(payload0);
        testData.insert_or_assign(lsn0, payload0);
        if (lsn0 != 0)
            return false;

        const vector<uint8_t> payload1 = {'t', 'e', 's', 't', 'i', 'n', 'g'};
        const auto lsn1 = wal.append(payload1);
        testData.insert_or_assign(lsn1, payload1);
        if (lsn1 != 1)
            return false;

        const vector<uint8_t> payload2 ={'s', 't', 'u', 'f', 'f'};
        const auto lsn2 = wal.append(payload2);
        testData.insert_or_assign(lsn2, payload2);
        if (lsn2 != 2)
            return false;

        const vector<uint8_t> payload3 = {'i', 's'};
        const auto lsn3 = wal.append(payload3);
        testData.insert_or_assign(lsn3, payload3);
        if (lsn3 != 3)
            return false;

        const vector<uint8_t> payload4 = {'F', 'U', 'N', '!'};
        const auto lsn4 = wal.append(payload4);
        testData.insert_or_assign(lsn4, payload4);
        if (lsn4 != 4)
            return false;
    }

    // OG wal obj is out of scope now -> create new one on OG dir and recover
    Wal wal{testDir};
    wal.recover();

    // Now validate all the test data
    for (const auto& it : testData) {
        if (it.second != wal.read(it.first)) {
            return false;
        }
    }

    // One final test case to validate the nextLsn_ recovery
    const std::vector<uint8_t> payload5 = {'a', 'g', 'a', 'i', 'n'};
    const auto lsn5 = wal.append(payload5);

    if (lsn5 != 5) {
        return false;
    }

    if (wal.read(lsn5) != payload5) {
        return false;
    }

    return true;
}

bool test_corruptTail() {
    const fs::path testDir = "test-data/corrupted-tail";
    if (fs::is_directory(testDir)) {
        fs::remove_all(testDir);
    }

    // store LSNs and their payloads to validate after recovery
    unordered_map<Lsn, vector<uint8_t>> testData;
    Lsn corruptedLsn = 4;

    // using block to force the wal object to go out of scope
    {
        Wal wal{testDir};
        wal.recover();

        const vector<uint8_t> payload0 = {'h', 'e', 'l', 'l', 'o'};
        const auto lsn0 = wal.append(payload0);
        testData.insert_or_assign(lsn0, payload0);

        const vector<uint8_t> payload1 = {'t', 'e', 's', 't', 'i', 'n', 'g'};
        const auto lsn1 = wal.append(payload1);
        testData.insert_or_assign(lsn1, payload1);

        const vector<uint8_t> payload2 ={'s', 't', 'u', 'f', 'f'};
        const auto lsn2 = wal.append(payload2);
        testData.insert_or_assign(lsn2, payload2);

        const vector<uint8_t> payload3 = {'i', 's'};
        const auto lsn3 = wal.append(payload3);
        testData.insert_or_assign(lsn3, payload3);

        const vector<uint8_t> payload4 = {'F', 'U', 'N', '!'};
        const auto lsn4 = wal.append(payload4);
        testData.insert_or_assign(lsn4, payload4);

        if (lsn4 != corruptedLsn)
            return false;
    }

    // resize file to make payload 4 partially complete
    fs::path filepath = testDir / "0.log";
    const auto originalSize = fs::file_size(filepath);
    fs::resize_file(filepath, originalSize - 3);

    Wal wal(testDir);
    wal.recover();

    // Now validate all the test data (except the corrupted payload)
    for (const auto& it : testData) {
        if (it.first != corruptedLsn && it.second != wal.read(it.first)) {
            return false;
        }
    }

    if (!wal.read(corruptedLsn).empty())
        return false;

    // validate that a new append uses LSN = 4
    const vector<uint8_t> finalPayload = {'f', 'i', 'n', 'a', 'l'};
    const auto lsnFinal = wal.append(finalPayload);

    if (lsnFinal != corruptedLsn)
        return false;

    if (wal.read(lsnFinal) != finalPayload)
        return false;

    return true;
}

bool test_badChecksum() {
    const fs::path testDir = "test-data/bad-checksum";
    if (fs::is_directory(testDir)) {
        fs::remove_all(testDir);
    }

    // store LSNs and their payloads to validate after recovery
    unordered_map<Lsn, vector<uint8_t>> testData;
    Lsn corruptedLsn = 4;

    // using block to force the wal object to go out of scope
    {
        Wal wal{testDir};
        wal.recover();

        const vector<uint8_t> payload0 = {'h', 'e', 'l', 'l', 'o'};
        const auto lsn0 = wal.append(payload0);
        testData.insert_or_assign(lsn0, payload0);

        const vector<uint8_t> payload1 = {'t', 'e', 's', 't', 'i', 'n', 'g'};
        const auto lsn1 = wal.append(payload1);
        testData.insert_or_assign(lsn1, payload1);

        const vector<uint8_t> payload2 ={'s', 't', 'u', 'f', 'f'};
        const auto lsn2 = wal.append(payload2);
        testData.insert_or_assign(lsn2, payload2);

        const vector<uint8_t> payload3 = {'i', 's'};
        const auto lsn3 = wal.append(payload3);
        testData.insert_or_assign(lsn3, payload3);

        const vector<uint8_t> payload4 = {'F', 'U', 'N', '!'};
        const auto lsn4 = wal.append(payload4);
        testData.insert_or_assign(lsn4, payload4);

        if (lsn4 != corruptedLsn)
            return false;
    }

    fs::path filepath = testDir / "0.log";
    const auto filesize = fs::file_size(filepath);

    if (filesize == 0) {
        return false;
    }

    // Pick a byte near the end of the final payload.
    const auto corruptOffset = static_cast<std::streamoff>(filesize - 1);

    std::fstream file(filepath, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    char byte = 0;
    file.seekg(corruptOffset, std::ios::beg);
    file.read(&byte, 1);

    if (!file) {
        return false;
    }

    // Flip bits so the payload changes but record length stays the same.
    byte ^= static_cast<char>(0xFF);

    file.seekp(corruptOffset, std::ios::beg);
    file.write(&byte, 1);

    if (!file) {
        return false;
    }

    file.close();

    Wal wal{testDir};
    wal.recover();

    // Now validate all the test data (except the corrupted payload)
    for (const auto& it : testData) {
        if (it.first != corruptedLsn && it.second != wal.read(it.first)) {
            return false;
        }
    }

    if (!wal.read(corruptedLsn).empty()) {
        cout << "This is the failing part!\n";
        return false;
    }

    // validate that a new append uses LSN = 4
    const vector<uint8_t> finalPayload = {'f', 'i', 'n', 'a', 'l'};
    const auto lsnFinal = wal.append(finalPayload);

    if (lsnFinal != corruptedLsn)
        return false;

    if (wal.read(lsnFinal) != finalPayload)
        return false;

    return true;
}

int main()
{
    if (!fs::is_directory("test-data")) {
        fs::create_directory("test-data");
    }
    cout << "\nTesting WAL implementation.\n\n";

    // TEST 1
    cout << "1. Starting Clean Restart test\n\n";
    if (!test_cleanRestart()) {
        cerr << "\n1. FAILED - Clean Restart\n";
        return 1;
    }
    else {
        cout << "\n1. PASSED\n";
    }

    // TEST 2
    cout << "\n2. Starting Corrupt Tail test\n\n";
    if (!test_corruptTail()) {
        cerr << "\n2. FAILED - Corrupt Tail\n";
        return 1;
    }
    else {
        cout << "\n2. PASSED\n";
    }

    // TEST 3
    cout << "\n3. Starting Bad Checksum test\n\n";
    if (!test_badChecksum()) {
        cerr << "\n3. FAILED - Bad Checksum\n";
        return 1;
    }
    else {
        cout << "\n3. PASSED\n";
    }
}