# DiskAch

A simple disk usage analyzer for Linux, written in C++20.

DiskAch recursively scans directories and displays their contents as a tree, including file and directory sizes, relative size bars, and basic scan statistics.

## Features

- Recursive directory scanning
- Scanning the current directory or any path passed as an argument
- Directory and file size calculation
- Tree-style output
- Relative size visualization
- Basic scan statistics
- Skipping selected directories (`.git`, `build`)

## Requirements

- Linux
- C++20 compiler with `std::format` support (GCC 13+ or Clang 17+)
- CMake 3.20 or newer

## Build

```bash
git clone https://github.com/ynshvrh/DiskAch.git
cd DiskAch

cmake -S . -B build
cmake --build build
```

## Usage

```bash
./build/DiskAch [path]
```

If no path is given, DiskAch scans the current working directory.

Examples:

```bash
./build/DiskAch
./build/DiskAch ~/Downloads
./build/DiskAch /var/log
./build/DiskAch ..
```

To run it from anywhere, copy the binary to a directory in your `PATH`:

```bash
mkdir -p ~/.local/bin
cp build/DiskAch ~/.local/bin/
```

## Tech Stack

- C++20
- CMake
- Linux Filesystem API (`std::filesystem`)

## Purpose

DiskAch is a small learning project created to explore modern C++20, filesystem operations, recursion, data structures, and CMake.

It is not intended to be a full-featured disk analysis utility.
