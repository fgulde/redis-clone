# Redis-Clone in C++

A learning project for building a simple Redis-like key-value store in **C++23**.

## Goal

This project is intended to explore how a Redis-style in-memory database can be implemented from scratch in C++.  
The focus is on learning and experimentation, not on replacing Redis.

Planned areas of exploration include:

- TCP networking
- Command parsing
- In-memory data structures
- Persistence basics
- Concurrency
- Client/server communication

## Current Status

This repository is in an early development stage.

At the moment, the project is being set up as a foundation for implementing:

- a lightweight Redis-style server
- a small command set
- simple client interactions
- extensible internal architecture

## Features to Explore

Potential features for this project may include:

- `SET` / `GET`
- key expiration
- basic lists or hashes
- persistence to disk
- command-line client
- simple protocol handling

## Tech Stack

- **Language:** C++23
- **Build System:** CMake
- **Package Management:** vcpkg

## Project Structure

The project is organized as a C++ application built with CMake.  
Additional source files and modules will be added as the implementation grows.

## Building

A typical build workflow may look like this:
