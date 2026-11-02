
# DiskAch

A simple disk usage analyzer for Linux, written in C++20.

DiskAch recursively scans directories and displays their contents as a tree, including file and directory sizes, relative size bars, and basic scan statistics.

## Features

- Recursive directory scanning
- Directory and file size calculation
- Tree-style output
- Relative size visualization
- Basic scan statistics
- Skipping selected directories

## Requirements

- Linux
- C++20 compiler
- CMake 3.20 or newer

## Build

```bash
git clone https://github.com/ynshvrh/DiskAch.git
cd DiskAch

cmake -S . -B build
cmake --build build
```

## Usage

Run DiskAch from the project directory:

```bash
./build/DiskAch
```

By default, DiskAch scans the current working directory.

## Tech Stack

- C++20
- CMake
- Linux Filesystem API (`std::filesystem`)

## Purpose

DiskAch is a small learning project created to explore modern C++20, filesystem operations, recursion, data structures, and CMake.

It is not intended to be a full-featured disk analysis utility.
