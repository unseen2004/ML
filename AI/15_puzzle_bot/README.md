# 15 Puzzle Solver

A C++ implementation of a solver for the classic 15 puzzle (sliding puzzle) using the A* search algorithm.

## Overview

This project implements a solution for the 15 puzzle (or 8 puzzle in 3x3 mode) using the A* search algorithm with two different heuristics:
- Hamming distance (number of misplaced tiles)
- Manhattan distance (sum of distances of tiles from their goal positions)

The solver can demonstrate individual puzzle solutions or run comparison tests between the two heuristics.

## Project Structure

```
.
├── CMakeLists.txt    # Build configuration file
└── main.cpp         # Main source code
```

## Build & Run

### Prerequisites
- C++20 compatible compiler
- CMake 3.30 or higher

### Build Instructions

```bash
# Create a build directory
mkdir build && cd build

# Configure and build
cmake ..
make

# Run the application
./15_puzzle_bot
```

### Usage

When running the application, you'll be prompted to:
1. Select the puzzle size (3x3 or 4x4)
2. Choose between:
    - Demonstrating a single random puzzle solution
    - Running performance tests comparing both heuristics

For the demonstration mode, you can select which heuristic to use. The solver will display the solution path, number of moves, states visited, and time taken.