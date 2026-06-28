# Replicated WAL

## Purpose of Project
The goal of this project is to demonstrate the following:

### Implemented
- C++17 systems programming
- multithreaded TCP server
- durable WAL storage
- crash recovery
- checksum validation
- deterministic tests

### Planned
- segment rotation
- leader/follower replication
- quorum-based commit

## Acronyms
WAL -> Write-Ahead Log

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
Completed in Milestone 1. The `tcp_integration_test` demonstrates this functionality. A better demo is planned in Milestone 6.

## Run a 3-node cluster
Planned for Milestone 4.

## Failure demos
Planned for Milestone 6.

## Correctness guarantees
Planned for Milestone 6.

## Current Status

Milestone 1 is complete.

The project currently supports a single-node durable WAL server. Clients can append records over TCP, concurrent client requests are serialized through one writer thread, records are persisted with checksums, and the server can recover valid records after restart.

Replication and leader/follower behavior are planned future milestones.

## Design Notes

### Milestone 1

A single-node durable WAL server.

A client can append records over TCP. The server writes records to disk using a binary WAL format with checksums. Multiple clients may append concurrently, but records are serialized by a dedicated writer thread. After a crash or restart, the server scans the WAL, validates checksums, discards partial records, and resumes from the last valid offset.

#### Complete

### Milestone 1.1: Single-node release hardening
#### Status
In progress.
#### Goal
Make Milestone 1 clean, testable, and demoable.
#### Features
1. Add shutdown mechanism
2. Add deeper integration tests
3. Add scripted demos
#### Completion criteria
1. Server supports graceful shutdown.
2. Shutdown does not interrupt an active WAL append.
3. Restart after graceful shutdown recovers all valid records.
4. Integration tests cover concurrent clients, restart recovery, corrupt tails, malformed clients, and oversized payloads.
5. Scripted demos can be run from the repository root.

### Milestone 2: Segment rotation + multi-segment recovery
#### Goal
Turn the WAL from "one durable file" into a real segmented log.

#### Features
```
wal-0000000000000000.log
wal-0000000000000001.log
wal-0000000000000002.log
```
1. Rotate when current segment reaches max size
2. Recover all valid segments on startup
3. Truncate only the corrupt tail segment
4. Ignore or remove later invalid segments
5. Maintain LSN -> segment/offset index
6. Read records across segment boundaries

#### Completion criteria

1. Appending enough records creates multiple segment files.
2. Restart rebuilds state from all valid segments.
3. Corrupting the final segment preserves earlier valid segments.
4. read(lsn) works even when the record is in an older segment.

### Milestone 3: Read/replay API + follower catch-up foundation
#### Goal
Expose the operation replication will need later: "give me records starting at this LSN."
#### Features
1. scanFrom(lsn)
2. scanRange(start_lsn, max_records)
3. client/demo command to fetch records
#### Completion criteria
1. The WAL can serve historical records by LSN after restart.
2. A future follower could catch up using only this API.

### Milestone 4: Static leader/follower replication
#### Goal
Replicate WAL records from one leader to one or more followers.
#### Features
1. One configured leader
2. Two configured followers
3. Clients append only to leader
4. Leader writes locally
5. Leader sends record to followers
6. Followers append replicated records
7. Followers reject direct client appends
#### Completion criteria
1. Start 3 nodes.
2. Append to leader.
3. Verify all nodes eventually contain the same records.
4. Kill follower.
5. Append more records.
6. Restart follower.
7. Follower catches up.

### Milestone 5: Quorum-based replicated WAL
#### Goal
Make replication correctness stronger.
#### Features
1. Leader sends append to followers
2. Record is committed only after majority acknowledgement
3. Client receives success only after quorum
4. Followers track durable replicated LSN
5. Leader tracks commit LSN
#### Completion criteria
1. In a 3-node cluster, one node can fail and appends still commit.
2. If the leader cannot reach a majority, appends do not commit.
3. Committed records survive restart.

### Milestone 6: Failure handling, rejoin, and demos
#### Goal
Make the project easy to understand and impressive to run.
#### Features
1. Scripted 3-node cluster startup
2. Scripted follower failure demo
3. Scripted leader failure limitation demo
4. Metrics/logging
5. Update/Create more README diagrams
6. Clear correctness guarantees
7. Clear limitations
#### Completion criteria
1. Someone can clone the repo, build it, run demos, and understand exactly what guarantee each demo proves.