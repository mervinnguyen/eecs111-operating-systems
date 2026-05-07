# EECS 111: System Software

This repository contains C++ assignments for EECS 111, a course on operating systems and system software. The projects focus on processes, threads, scheduling, synchronization primitives, deadlocks, and resource management.

## Projects

- `project1/` - process-based and thread-based parallel sorting
- `project2/` - thread synchronization around a shared resource room

## Requirements

- `g++` with C++98 support
- `make`
- POSIX threads (`pthread`)
- `python3` for the Project 1 autograder

## Build and Run

### Project 1

```bash
cd project1/project1/p1
make
./p1_exec <number_of_processes> <number_of_threads>
```

### Project 2

```bash
cd project2
make
./p2_exec <number>
```

For Project 2, the argument is the number of Faculty Members and the number of Students to simulate.

## Cleaning

```bash
cd project1/project1/p1 && make clean
cd project2 && make clean
```
