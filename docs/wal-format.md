# WAL Format

## Purpose
The purpose of defining an explicit Write-Ahead Log format is to support atomicity, durability, crash recovery, and data integrity.

The WAL format defines how records are written to disk, how each record is identified, and how each record is validated during recovery. This allows the server to distinguish complete records from partial or corrupted records after a crash.

During recovery, the server scans the WAL in order and only accepts records with a valid header, valid length, and valid checksum. If recovery finds a partial record, corrupted record, or invalid record header, it stops scanning and truncates the invalid tail of the log.

This ensures that recovery is deterministic and that the server resumes from the last complete, valid WAL record.

## WAL Directory Layout

The WAL directory contains one or more `.log` files. Each `.log` file is called a **segment**.

Example WAL directory:

```
data/
  wal-0000000000000001.log
  wal-0000000000000002.log
  wal-0000000000000003.log
```

During recovery, the server scans segment files in ascending order.

## Segment Files
Each segment contains zero or more WAL records.
```
+----------+----------+----------+----------+
| Record 1 | Record 2 | Record 3 | Record N |
+----------+----------+----------+----------+
```

## Record Layout

Each WAL **record** is stored as a fixed-size **record header** followed by a variable-size **record payload**.

```text
+------------+---------------------+---------------+---------------+-------------------------+
| magic      | payload_len         | lsn           | crc           | payload                 |
| 4 bytes    | 4 bytes             | 8 bytes       | 4 bytes       | payload_len bytes       |
+------------+---------------------+---------------+---------------+-------------------------+
```

## Record Header Fields

The **record header** contains the metadata needed to identify, order, and validate a WAL record during normal reads and crash recovery.

### `magic`

A fixed 4-byte value that identifies the start of a WAL record.

The magic value allows the recovery scanner to determine whether it is positioned at the beginning of a valid record. If the magic value does not match the expected constant, recovery stops and the remaining tail of the segment is considered invalid.

Example:

```cpp
constexpr std::uint32_t magic = 0x314C4157; // WAL1 - litte endian
```

### `payload_len`

A 4-byte unsigned integer that stores the number of bytes in the record payload.

The recovery scanner uses this field to determine how many payload bytes must be read after the header. If the segment ends before `payload_len` bytes are available, the record is considered partial and recovery stops.

### `lsn`

An 8-byte unsigned integer containing the Log Sequence Number for the record.

The `lsn` field is assigned by the WAL writer thread and increases monotonically for each successfully appended record. It provides a stable ordering for records in the log.

### `crc`

A 4-byte CRC32 checksum used to validate record integrity.

The checksum is calculated over the record data, excluding the `crc` field itself. During recovery, the checksum is recomputed and compared with the stored value. If the values do not match, the record is considered corrupted and recovery stops.

### `payload`

The raw record data submitted by the client or storage engine.

For the initial WAL implementation, the payload is treated as opaque bytes. The WAL layer does not interpret the payload contents.

## LSN Assignment

Each WAL record is assigned a Log Sequence Number, or LSN.

An LSN is a monotonically increasing 64-bit unsigned integer assigned by the WAL writer thread. LSNs are global across all segment files. They do not reset when a new segment is created.

Example:

```text
wal-0000000000000001.log
  LSN 1
  LSN 2
  LSN 3

wal-0000000000000002.log
  LSN 4
  LSN 5
  LSN 6
```
**segment file** = physical storage location

**LSN** = logical record order

## Checksum Rules

Each WAL record stores a CRC32 checksum in the `crc` field of the record header. The checksum is used during recovery to verify that the record header and payload were written completely and have not been corrupted.

The checksum is calculated over the following fields, in this exact order:

```text
magic
payload_len
lsn
payload
```

The `crc` field itself is not included in the checksum calculation.

Integer fields are serialized into their on-disk byte representation before being added to the checksum. The checksum must be calculated over the exact bytes written to disk, not over the in-memory C++ representation of a struct.

```text
crc = CRC32(magic || payload_len || lsn || payload)
```

During recovery, the server reads a record header and payload, recalculates the CRC32 value using the same rules, and compares the recalculated value with the stored `crc` field.

If the recalculated checksum does not match the stored checksum, the record is considered invalid. Recovery stops at the first invalid record. The invalid record and all later records are discarded because the WAL is an ordered append-only log.

If the invalid record is found in a segment file, that segment is truncated at the byte offset where the invalid record begins. Any later segment files are also discarded.


## Segment Rotation

To prevent unbounded segment growth, the WAL is split into multiple segment files. The WAL writer appends to one active segment at a time.

When writing a new record would cause the active segment to exceed `max_segment_len`, the WAL writer closes the current segment and creates the next segment file.

Segment rotation allows the WAL to be managed in chunks, which helps with recovery, deletion, copying, testing, debugging, and future replication.

A record is never split across multiple segment files. If a record does not fit in the active segment, the writer rotates to a new segment before writing the record.


## Recovery Rules

Server recovery runs during startup before the server begins accepting client requests. The server does not assume that the previous shutdown was clean, so it validates the WAL every time it starts.

Recovery scans WAL segment files in ascending segment order. Within each segment, records are scanned in the order they appear on disk.

For each record, recovery validates:

1. The record header can be fully read.
2. The `magic` value matches the expected WAL record magic value.
3. The `payload_len` value is valid and the full payload can be read.
4. The recalculated CRC32 checksum matches the stored `crc` value.
5. The record LSN follows the expected ordering rules.

If all checks pass, the record is accepted as valid and recovery advances to the next record.

If any validation check fails, recovery stops at that record. The failed record and all later records are considered invalid because the WAL is an ordered append-only log.

When recovery stops inside a segment file, the server truncates that segment at the byte offset where the invalid record begins. Any later segment files are discarded.

After recovery completes, the server resumes appending at the next LSN after the last valid record.


## Initial Limitations
This initial WAL format intentionally does not include a version field. The project currently supports one record format only. If the WAL format needs to evolve in the future, a version field can be added as part of a planned format migration.