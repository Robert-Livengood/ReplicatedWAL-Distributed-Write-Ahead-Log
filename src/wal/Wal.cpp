#include "Wal.h"
#include "Record.h"

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <algorithm>

using namespace std;
namespace fs = std::filesystem;

Wal::Wal(const fs::path dataDir) {
    cout << "Constructing wal object\n";
    nextLsn_ = 0;
    dataDir_ = dataDir;
}

Lsn Wal::append(const vector<uint8_t>& payload) {
    Lsn lsn = nextLsn_;
    nextLsn_++;
    
    error_code ec;
    if (fs::exists(dataDir_, ec)) {
        // TODO -> add padding to the segment file names
        // TODO -> need to handle multiple segment files eventually
        string filename = to_string(0)+ ".log";
        fs::path filepath = dataDir_ / filename;

        ofstream file(filepath, ios::app | ios::binary);

        if (!file.is_open()) {
            cerr << "Error opening the file!\n";
            exit(1);
        }

        // save file position -> this is the start of the record
        file.seekp(0, ios::end);
        streampos filePosition = file.tellp();

        writeRecord(file, lsn, payload);

        RecordLocation recordData;
        recordData.segmentPath = filepath;
        recordData.offset = static_cast<uint64_t>(static_cast<streamoff>(filePosition));

        // update file position -> this is the end of the record
        filePosition = file.tellp();
        recordData.recordSize = static_cast<uint64_t>(static_cast<streamoff>(filePosition)) - recordData.offset;

        // save the records data in the LSN map
        lsnMap.insert_or_assign(lsn, recordData);

        file.close();
    }
    else {
        cerr << "Error with data dir: " << ec << "\n";
        exit(1);
    }

    return lsn;
}

vector<uint8_t> Wal::read(Lsn lsn) const {
    cout << "Reading record\n";

    auto it = lsnMap.find(lsn);
    if (it == lsnMap.end())
        return {};

    RecordLocation recordData = it->second;

    ifstream file(recordData.segmentPath, ios::binary);

    RecordReadResult res;
    try {
        res = readRecord(file, recordData.offset, fs::file_size(recordData.segmentPath));
    }
    catch (...) {
        res.status = RecordReadStatus::Corrupt;
    }

    if (res.nextOffset != recordData.offset + recordData.recordSize) {
        return {};
    }

    if (res.status == RecordReadStatus::Ok && res.lsn == lsn)
        return res.payload;
    else
        return {};
}

void Wal::recover() {
    nextLsn_ = 0;
    lsnMap.clear();

    cout << "Recovering\n";

    if (!dataDir_.empty() && !fs::exists(dataDir_)) {
        cout << "Directory " << dataDir_ << " does not exist. Creating it now.\n";
        fs::create_directory(dataDir_);
    }
    else {
        // segments exist... need to:
        // 1. scan through segments
        // 2. check for errors
        // 3. truncate corrupt records
        // 4. update nextLsn_ based on last valid LSN + 1

        // collect and sort segment files so the order is deterministic
        vector<fs::path> segmentFiles;
        for (const auto& entry : fs::directory_iterator(dataDir_)) {
            if (!entry.is_regular_file())
                continue;

            segmentFiles.push_back(entry.path());
        }
        sort(segmentFiles.begin(), segmentFiles.end());

        // once corruption is found the current segment is truncatedted at the first
        // this means the remaining segments cannot be trusted -> remove them.
        bool foundCorruption = false;

        for (const auto& segmentFile : segmentFiles){
            // If the last segment had corrupt records -> remove the rest of the files
            if (foundCorruption) {
                error_code ec;
                fs::remove(segmentFile, ec);

                if (ec) {
                    cerr << "Error removing file: " << ec.message() << "\n";
                }
                continue;
            }
            
            RecordReadResult res;
            res.status = RecordReadStatus::EndOfFile;
            uint64_t offset = 0;

            ifstream file(segmentFile, ios::binary);

            if (!file.is_open()) {
                cerr << "Error opening the file! " << segmentFile << "\n";
                exit(1);
            }

            uintmax_t filesize = fs::file_size(segmentFile);
            if (filesize > UINT64_MAX)
                throw std::runtime_error("File size is too large");

            uint64_t recordStart = offset;

            while (offset < filesize) {
                // save the start of the record in case corruption is found -> this allows the prev good records to be retained
                recordStart = offset;

                try {
                    res = readRecord(file, offset, filesize);
                }
                catch (...){
                    res.status = RecordReadStatus::Corrupt;
                }

                if (res.status == RecordReadStatus::Ok) {
                    nextLsn_ = max(nextLsn_, res.lsn + 1);
                    offset = res.nextOffset;

                    RecordLocation recordData;
                    recordData.segmentPath = segmentFile;
                    recordData.offset = recordStart;
                    recordData.recordSize = offset - recordStart;
                    lsnMap.insert_or_assign(res.lsn, recordData);
                }
                else if (res.status == RecordReadStatus::Corrupt) {
                    break;
                }
            }

            if (res.status == RecordReadStatus::Corrupt) {
                file.close();
                error_code ec;
                fs::resize_file(segmentFile, recordStart, ec);

                if (ec) {
                    cerr << "Error truncating file: " << ec.message() << "\n";
                }
                foundCorruption = true;
                continue;
            }

            if (file.is_open())
                file.close();
        }
    }
}