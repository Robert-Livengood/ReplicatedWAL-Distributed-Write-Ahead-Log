import shutil
import socket
import time
import struct
import zlib
import subprocess
from pathlib import Path

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

def startServer(server_cmd, cwd, host, port, timeout_seconds=5):
    server_proc = subprocess.Popen(
        server_cmd,
        cwd=cwd,
        stdin=subprocess.PIPE,
        text=True
    )

    try:
        waitForServer(host, port, timeout_seconds)

        if server_proc.poll() is not None:
            raise RuntimeError(
                f"Server exited unexpectedly after startup with code {server_proc.returncode}"
            )

        return server_proc

    except Exception:
        stopServerGracefully(server_proc)
        raise

def stopServerGracefully(server_proc, timeout_seconds=5):
    if server_proc is None:
        return

    if server_proc.poll() is not None:
        return

    try:
        if server_proc.stdin is not None:
            server_proc.stdin.write("\n")
            server_proc.stdin.flush()

        server_proc.wait(timeout=timeout_seconds)

    except (BrokenPipeError, OSError, subprocess.TimeoutExpired):
        server_proc.terminate()

        try:
            server_proc.wait(timeout=timeout_seconds)
        except subprocess.TimeoutExpired:
            server_proc.kill()
            server_proc.wait()

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

def computeCRC(magic, payload_len, lsn, payload):
    crc_bytes = bytearray()
    crc_bytes += magic
    crc_bytes += struct.pack("<I", payload_len)
    crc_bytes += struct.pack("<Q", lsn)
    crc_bytes += payload

    return zlib.crc32(crc_bytes) & 0xFFFFFFFF

def verifyRecords(path: Path, numClients: int):
    # verify file size
    file_size = path.stat().st_size

    expected_payloads = {f"Hello from client {i}".encode("utf-8") for i in range(numClients)}
    expected_file_size = sum(HEADER_SIZE + len(payload) for payload in expected_payloads)

    if file_size != expected_file_size:
        raise RuntimeError("Test log file is not the expected size")
    else:
        print(f"Test log file is the expected size:\n Expected: {expected_file_size}\n Actual: {file_size}")

    # verify the record contents
    records = parseLogFile(path)

    if len(records) != numClients:
        raise RuntimeError(f"Number of records does not match expected value: Expected: {numClients}\n Actual: {len(records)}\n")
    
    lsns = []
    actual_payloads = set()
    expected_payloads = set(expected_payloads)

    for record in records:
        if not record["crc_ok"]:
            raise RuntimeError(f"Computed CRC does not match stored CRC at offset: {record['offset']}")
        lsns.append(record["lsn"])
        actual_payloads.add(record["payload"])


    if len(set(lsns)) != numClients:
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

def cleanTestDir(path: Path):
    if path.exists():
        shutil.rmtree(path)

    path.resolve().parent.mkdir(parents=True, exist_ok=True)