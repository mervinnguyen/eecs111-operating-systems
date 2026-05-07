# EECS 111: System Software

This repository contains C++ assignments for EECS 111, a course on operating systems and system software. The projects focus on processes, threads, scheduling, synchronization primitives, deadlocks, and resource management.

## Overview

The repository is organized around course projects completed over time:

- `project1/` implements a multi-process, multi-threaded sorting pipeline.
- `project2/` simulates coordinated access to a shared resource room using mutexes and condition variables.

Additional projects can be added in new top-level folders as the course continues.

## Repository Structure

- `project1/project1/p1/` - source code, input files, autograder, and generated outputs for Project 1
- `project2/` - source code, build files, and questions for Project 2
- `projectN/` - future projects can follow the same pattern with their own build and run instructions

## Requirements

- `g++` with C++98 support
- `make`
- POSIX threads (`pthread`)
- `python3` for the Project 1 autograder

## Build Instructions

### Project 1

```bash
cd project1/project1/p1
make
./p1_exec <number_of_processes> <number_of_threads>
```
This program reads the CSV files in `input/`, distributes work across child processes, and sorts each dataset with worker threads.

### Project 2

```bash
cd project2
make
./p2_exec <number>
```

The argument is the number of Faculty Members and the number of Students to simulate. The program creates `2 * number` total people with randomized arrival order.

## Cleaning Build Artifacts

```bash
cd project1/project1/p1 && make clean
cd project2 && make clean
```

## Troubleshooting

- If a build fails, confirm that `g++`, `make`, and `pthread` support are installed.
- If Project 1 is run from the wrong directory, the CSV input files will not be found.
- If Project 2 is run with a non-positive argument, the program will print a usage error.

## License

Coursework repository. No separate license has been provided.
