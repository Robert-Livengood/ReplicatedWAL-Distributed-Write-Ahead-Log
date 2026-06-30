import subprocess
from pathlib import Path

import test_utils

ROOT = Path(__file__).resolve().parents[1]
BUILD_DIR = ROOT / "build"

CLIENT_EXE = BUILD_DIR / "Debug" / "wal_client_demo.exe"
SERVER_EXE = BUILD_DIR / "Debug" / "wal_server_demo.exe"

TEST_DATA_DIR = ROOT / "test-data" / "tcp-test"
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

    # First startup/shutdown cycle: no clients
    server_proc = test_utils.startServer(server_cmd, ROOT, HOST, PORT)

    try:
        if server_proc.poll() is None:
            print("Server1 started")
        else:
            raise RuntimeError("Server1 did not start!")
    finally:
        test_utils.stopServerGracefully(server_proc)

    if server_proc.poll() == 0:
        print("Server1 shutdown successfully\n")
    else:
        raise RuntimeError("Server1 shutdown failed!")
    
    # Second startup/shutdown cycle: run clients, then stop
    server_proc2 = test_utils.startServer(server_cmd, ROOT, HOST, PORT)

    try:
        if server_proc2.poll() is None:
            print("Server2 started")
        else:
            raise RuntimeError("Server2 did not start!")
        
        subprocess.run(client_cmd, cwd=ROOT, check=True)

    finally:
        test_utils.stopServerGracefully(server_proc2)

    if server_proc2.poll() == 0:
        print("Server2 shutdown\n")
    else:
        raise RuntimeError("Server2 shutdown failed!")
    
    test_utils.verifyRecords(TEST_LOG, NUM_CLIENTS)


if __name__ == "__main__":
    main()