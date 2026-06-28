# Replicated WAL

## Purpose of Project
The goal of this project is to demonstrate the following:

- C++17 systems programming
- multithreading TCP server
- durable WAL storage
- crash recovery
- checksum validation
- leader/follower replication
- deterministic tests

## Acronyms
WAL -> Write Ahead Log

LSN -> Log Sequence Number

## Architecture

## Build
Run from repository root:
```
cmake -S . -B build
cmake --build build
```

## Verification Tests
### WAL Integration Tests
From repository root:
```
.\build\Debug\wal_tests.exe
``` 
### TCP Integration Test
From repository root:
```
python .\tests\tcp_integration_test.py
```

## Run a single node

## Run a 3-node cluster

## Failure demos

## Correctness guarantees

## Current Limitations
- Project is in prototype stage. Nothing much to show yet.
- Current implementation is single-node only. 

## Design Notes

### Milestone 1

A single-node durable WAL server.

A client can append records over TCP. The server writes records to disk using a binary WAL format with checksums. Multiple clients may append concurrently, but records are serialized by a dedicated writer thread. After a crash or restart, the server scans the WAL, validates checksums, discards partial records, and resumes from the last valid offset.

#### Complete