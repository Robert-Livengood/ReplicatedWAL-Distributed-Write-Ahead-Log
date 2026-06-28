import os
import shutil
import socket
import subprocess
import time
from pathlib import Path
import struct
import zlib

ROOT = Path(__file__).resolve().parents[1]
BUILD_DIR = ROOT / "build"

CLIENT_EXE = BUILD_DIR / "Debug" / "wal_client_demo.exe"
SERVER_EXE = BUILD_DIR / "Debug" / "wal_server_demo.exe"

TEST_DATA_DIR = ROOT / "test-data" / "tcp-test"
TEST_LOG = TEST_DATA_DIR / "0.log"

HOST = "127.0.0.1"
PORT = 8080
NUM_CLIENTS = 10

HEADER_SIZE = 20
MAGIC = b"WAL1"

def waitForServer(host, port, timeout_seconds=5):
    deadline = time.time() + timeout_seconds

    while time.time() < deadline:
        try:
            with socket.create_connection((host, port), timeout=0.25):
                return
        except OSError:
            time.sleep(0.1)
    
    raise RuntimeError("Server did not start listening in time")

def computeCRC(magic, payload_len, lsn, payload):
    crc_bytes = bytearray()
    crc_bytes += magic
    crc_bytes += struct.pack("<I", payload_len)
    crc_bytes += struct.pack("<Q", lsn)
    crc_bytes += payload

    return zlib.crc32(crc_bytes) & 0xFFFFFFFF

def parseLogFile(path):
    records = []

    with open(path, "rb") as f:
        data = f.read()

    offset = 0

    while offset < len(data):
        if offset + HEADER_SIZE > len(data):
            raise RuntimeError(f"incomplete header at offset: {offset}")
        
        header = data[offset:offset + HEADER_SIZE]

        magic, payload_len, lsn, stored_crc = struct.unpack("<4sIQI", header)

        if magic != MAGIC:
            raise RuntimeError(f"Invalid magic at offset: {offset}: {magic}")
        
        payload_start = offset + HEADER_SIZE
        payload_end = payload_start + payload_len

        if payload_end > len(data):
            raise RuntimeError(f"incomplete payload at offset: {offset}")
        
        payload = data[payload_start:payload_end]

        computed_crc = computeCRC(magic, payload_len, lsn, payload)

        records.append({
            "offset": offset,
            "lsn": lsn,
            "payload": payload,
            "stored_crc": stored_crc,
            "computed_crc": computed_crc,
            "crc_ok": stored_crc == computed_crc
        })

        offset = payload_end

    return records


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
    server_proc = subprocess.Popen(server_cmd, cwd=ROOT, stdin=subprocess.PIPE, text=True)

    try:
        waitForServer(HOST, PORT)
        if server_proc.poll() is not None:
            raise RuntimeError("Server exited unexpectedly after startup")

        subprocess.run(client_cmd, cwd=ROOT, check=True)

        # Verify that the test log exists
        if not TEST_LOG.exists():
            raise RuntimeError("Test log file is not created")
        else:
            print(f"Test log file {TEST_LOG} is created\n")

        # verify file size
        file_size = TEST_LOG.stat().st_size

        expected_payloads = {f"Hello from client {i}".encode("utf-8") for i in range(NUM_CLIENTS)}
        expected_file_size = sum(HEADER_SIZE + len(payload) for payload in expected_payloads)

        if file_size != expected_file_size:
            raise RuntimeError("Test log file is not the expected size")
        else:
            print(f"Test log file is the expected size:\n Expected: {expected_file_size}\n Actual: {file_size}")

        # verify the record contents
        records = parseLogFile(TEST_LOG)

        if len(records) != NUM_CLIENTS:
            raise RuntimeError(f"Number of records does not match expected value: Expected: {NUM_CLIENTS}\n Actual: {len(records)}\n")
        
        lsns = []
        actual_payloads = set()
        expected_payloads = set(expected_payloads)

        for record in records:
            if not record["crc_ok"]:
                raise RuntimeError(f"Computed CRC does not match stored CRC at offset: {record['offset']}")
            lsns.append(record["lsn"])
            actual_payloads.add(record["payload"])


        if len(set(lsns)) != NUM_CLIENTS:
            raise RuntimeError("Number of unique LSNs is not correct")
        
        expected_lsn = 0
        for lsn in lsns:
            if lsn != expected_lsn:
                raise RuntimeError("LSNs are not in incrementing order")
            
            expected_lsn += 1

        if expected_payloads != actual_payloads:
            missing = expected_payloads - actual_payloads
            unexpected = actual_payloads - expected_payloads
            raise RuntimeError(f"Payload mismatch. Missing: {missing}, Unexpected: {unexpected}")
            
        print("\nAll Tests PASSED!")

    finally:
        if server_proc.poll() is None:
            try:
                server_proc.stdin.write("\n")
                server_proc.stdin.flush()
                server_proc.wait(timeout=5)
            except:
                server_proc.terminate()

                try:
                    server_proc.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    server_proc.kill()
                    server_proc.wait()


if __name__ == "__main__":
    main()