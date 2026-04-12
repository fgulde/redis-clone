# Redis-Clone in C++

A Redis-compatible server implementation built from scratch in **C++23**.

## Goal

This project explores how a Redis-style in-memory database can be implemented from scratch in C++.  
The focus is on learning and understanding the internals of Redis.

Areas of exploration include:

- TCP networking & socket programming
- The RESP (Redis Serialization Protocol) wire format
- Command parsing
- In-memory data structures
- Concurrency & event loops
- Client/server communication

## Current Status

Already implemented:
- RESP2 protocol parsing
- `PING`, `ECHO`, `SET`, `GET` commands
- Basic client/server communication

Planned:
- Key expiration
- Persistence to disk
- Basic lists and hashes

## Tech Stack

- **Language:** C++23
- **Build System:** CMake
- **Package Management:** vcpkg

## Building
```bash
cmake -B build
cmake --build build
./build/server
```

You can then connect with any Redis client:
```bash
redis-cli ping
redis-cli -p 6379
```

## Project Structure
```
src/
└── main.cpp   # Entry point and server logic
```