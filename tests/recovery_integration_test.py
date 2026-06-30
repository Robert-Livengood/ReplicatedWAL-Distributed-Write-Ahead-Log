import subprocess
from pathlib import Path

import test_utils

ROOT = Path(__file__).resolve().parents[1]
BUILD_DIR = ROOT / "build"

CLIENT_EXE = BUILD_DIR / "Debug" / "wal_client_demo.exe"
SERVER_EXE = BUILD_DIR / "Debug" / "wal_server_demo.exe"

TEST_DATA_DIR = ROOT / "test-data" / "recovery-test"
TEST_LOG = TEST_DATA_DIR / "0.log"

HOST = "127.0.0.1"
PORT = 8080
NUM_CLIENTS = 10

def main():
    # verify that the executables are created
    if not BUILD_DIR.exists():
        raise RuntimeError("build dir does not exist, run cmake -S . -B build first")
    
    if not CLIENT_EXE.exists() or not SERVER_EXE.exists():
        subprocess.run(["cmake", "--build", str(BUILD_DIR)], cwd=ROOT, check=True)

    test_utils.cleanTestDir(TEST_DATA_DIR)

    server_cmd = [str(SERVER_EXE), str(TEST_DATA_DIR), str(PORT)]
    client_cmd = [str(CLIENT_EXE), str(HOST), str(PORT), str(NUM_CLIENTS)]

    # start server
    server_proc = test_utils.startServer(server_cmd, ROOT, HOST, PORT)

    # run 10 clients
    try:
        subprocess.run(client_cmd, cwd=ROOT, check=True)

        if not TEST_LOG.exists():
            raise RuntimeError("Test log file is not created")
    
    finally:
        # gracefully stop server
        test_utils.stopServerGracefully(server_proc)

    # start server again using same data dir
    server_proc = test_utils.startServer(server_cmd, ROOT, HOST, PORT)
    
    # run 10 more clients
    try:
        subprocess.run(client_cmd, cwd=ROOT, check=True)

        if not TEST_LOG.exists():
            raise RuntimeError("Test log file is not created")
    
    finally:
        # gracefully stop server
        test_utils.stopServerGracefully(server_proc)

    # parse log
    # verify 20 records exist
    # verify all payloads exist
    # verify CRCs are valid
    # verify LSN behavior is correct
    test_utils.verifyRecords(TEST_LOG, NUM_CLIENTS, 2) # 2 -> Client command ran twice
    

if __name__ == "__main__":
    main()