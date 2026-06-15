#include "Wal.h"
#include "Record.h"

#include <iostream>
#include <fstream>
#include <string>

Wal::Wal(const fs::path dataDir) {
    cout << "Constructing wal object\n";
    nextLsn_ = 0;
    dataDir_ = dataDir;
}

Lsn Wal::append(const vector<uint8_t>& payload) {
    Lsn lsn = nextLsn_;
    nextLsn_++;
    
    cout << "Creating segment file\n";
    error_code ec;
    if (fs::exists(dataDir_, ec)) {
        string filename = to_string(0)+ ".log"; // TODO - need to handle multiple segment files eventually
        fs::path filepath = dataDir_ / filename;

        ofstream file(filepath, ios::app | ios::binary);

        if (!file.is_open()) {
            cerr << "Error opening the file!\n";
            exit(1);
        }

        writeRecord(file, lsn, payload);

        file.close();
    }
    else {
        cerr << "Error with data dir: " << ec << "\n";
        exit(1);
    }

    cout << "Returning nextLsn_: " << lsn << endl;
    return lsn;
}

vector<uint8_t> Wal::read(Lsn lsn) const {
    cout << "Reading record\n";
    return *new vector<uint8_t>();
}

void Wal::recover() {
    cout << "Recovering\n";
    if (!dataDir_.empty() && !fs::exists(dataDir_)) {
        cout << "Directory " << dataDir_ << " does not exist. Creating it now.\n";
        fs::create_directory(dataDir_);
    }
    else {
        // segments exist... need to:
        // 1. scan through _get_thread_local_invalid_parameter_handler
        // 2. check for errors
        // 3. truncate corrupt records
        // 4. update nextLsn_ based on last valid LSN + 1
        for (const auto& entry : fs::recursive_directory_iterator(dataDir_)){
            RecordReadResult res;
            uint64_t offset = 0;

            ifstream file(entry.path(), ios::binary);

            if (!file.is_open()) {
                cerr << "Error opening the file! " << entry.path() << "\n";
                exit(1);
            }

            uintmax_t filesize = fs::file_size(entry.path());
            if (filesize > UINT64_MAX)
                throw std::runtime_error("File size is too large");

            while (offset < filesize) {
                res = readRecord(file, offset);
                // TODO - more recovery steps will be needed here eventually
                if (res.status == RecordReadStatus::Ok) {
                    nextLsn_ = res.lsn + 1;
                    offset = res.nextOffset;
                }
            }
            if (res.status == RecordReadStatus::Corrupt){} // TODO - need to add corrupt record handling
        }
    }
}