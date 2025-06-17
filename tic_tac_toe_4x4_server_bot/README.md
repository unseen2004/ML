# Tic-Tac-Toe 5x5 Client-Server Game

A networked implementation of a 5x5 Tic-Tac-Toe variant with special winning/losing conditions.

## Game Rules

- Game is played on a 5×5 board
- Players take turns placing X or O
- Win by connecting 4 marks in a row (horizontally, vertically, or diagonally)
- Lose by creating a 3-in-a-row that isn't part of a potential 4-in-a-row
- Game ends in draw after 25 moves (full board)

## Directory Structure

```
.
├── minimax_player.cpp  # Enhanced AI with human play option
└── server
    ├── board.hpp       # Game board implementation
    ├── CMakeLists.txt  # Build configuration
    ├── game_client.cpp # Human player client
    └── game_random_bot.cpp # Simple random move bot
```

## Build Instructions

### Server and Basic Clients

```bash
cd server
mkdir build && cd build
cmake ..
make
```

### Minimax AI Clients

```bash
g++ -o minimax_client minimax_client.cpp -O3
g++ -o minimax_player minimax_player.cpp -O3
```

## How to Run

### Start the Server

```bash
cd server/build
./game_server <IP> <PORT>
# Example: ./game_server 127.0.0.1 8080
```

### Connect with Human Client

```bash
cd server/build
./game_client <IP> <PORT> <PLAYER_TYPE> <NAME>
# Example: ./game_client 127.0.0.1 8080 1 Player1
# PLAYER_TYPE: 1=X, 2=O
```

### Connect with Random Bot

```bash
cd server/build
./game_random_bot <IP> <PORT> <PLAYER_TYPE> <NAME>
# Example: ./game_random_bot 127.0.0.1 8080 2 RandomBot
```

### Connect with Interactive Minimax Player or AI

```bash
./minimax_player <IP> <PORT> <PLAYER_TYPE> <NAME> [DEPTH] [AI]
# Example: ./minimax_player 127.0.0.1 8080 2 Player 5 1
# DEPTH: AI search depth (1-10), default=5
# AI: 0=human mode, 1=AI mode (default=0)
```

Note: Two players of different types (1 and 2) must connect to start a game.