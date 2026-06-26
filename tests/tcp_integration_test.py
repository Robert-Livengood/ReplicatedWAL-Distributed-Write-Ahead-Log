import os
import shutil
import socket
import subprocess
import time
from pathlib import Path

# TODO need better shutdown mechanism, but this works for now

ROOT = Path(__file__).resolve().parents[1]
BUILD_DIR = ROOT / "build"

CLIENT_EXE = BUILD_DIR / "Debug" / "wal_client_demo.exe"
SERVER_EXE = BUILD_DIR / "Debug" / "wal_server_demo.exe"

TEST_DATA_DIR = ROOT / "test-data" / "tcp-test"
TEST_LOG = TEST_DATA_DIR / "0.log"

HOST = "127.0.0.1"
PORT = 8080
NUM_CLIENTS = 10

def waitForServer(host, port, timeout_seconds=5):
    deadline = time.time() + timeout_seconds

    while time.time() < deadline:
        try:
            with socket.create_connection((host, port), timeout=0.25):
                return
        except OSError:
            time.sleep(0.1)
    
    raise RuntimeError("Server did not start listening in time")

def main():
    # Check the executables and clear the test dir
    if not BUILD_DIR.exists():
        raise RuntimeError("build dir does not exist, run cmake -S . -B build first")
    
    if not CLIENT_EXE.exists() or not SERVER_EXE.exists():
        subprocess.run(["cmake", "--build", str(BUILD_DIR)], cwd=ROOT, check=True)

    if TEST_DATA_DIR.exists():
        shutil.rmtree(TEST_DATA_DIR)

    server_cmd = [str(SERVER_EXE), str(TEST_DATA_DIR), str(PORT)]
    client_cmd = [str(CLIENT_EXE), str(HOST), str(PORT), str(NUM_CLIENTS)]

    # first init server
    server_proc = subprocess.Popen(server_cmd, cwd=ROOT)

    try:
        waitForServer(HOST, PORT)

        subprocess.run(client_cmd, cwd=ROOT, check=True)

        # TODO verify tests


    finally:
        server_proc.terminate()

        try:
            server_proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            server_proc.kill()
            server_proc.wait()


if __name__ == "__main__":
    main()