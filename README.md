# Networked Battleship in C++

## Project Description

This project is a complete Battleship game implemented entirely in C++. It contains a Linux/WSL client-server version of the game where multiple players connect through sockets, wait in a matchmaking queue, and play turn-based Battleship matches from the terminal.

The game was built as a networked version of the classic Battleship board game. Each player hides a fleet on a 10x10 grid and takes turns attacking coordinates until one player remains. The server manages connections, matchmaking, turn order, game state, and win/lose handling.

## What This Project Demonstrates

- A complete game written in C++
- Client-server communication using socket programming
- Object-oriented design for ships, boards, players, and match sessions
- Multithreading for accepting clients and running matches
- Queue-based matchmaking with synchronization using mutex and condition variable
- Clean separation between protocol, game logic, server logic, and client logic

## Applied Concepts

- Object-Oriented Programming (classes, encapsulation, and modular design)
- Socket Programming (TCP server/client communication)
- Multithreading with `std::thread`
- Synchronization with `std::mutex` and `std::condition_variable`
- Random fleet placement using C++ standard library utilities
- Modular code organization with reusable headers

## How The Game Works

1. Start the server on your machine.
2. Open one or more client terminals and connect to the server.
3. Players are placed into a match when enough players are available.
4. Each player gets a hidden board with ships placed automatically.
5. On your turn, enter an attack coordinate such as `A5`.
6. In matches with more than two players, the attack format becomes `ATTACK <target> <coord>` where `target` is the opponent index.
7. The server replies with results like `HIT`, `MISS`, `SUNK`, `WIN`, or `LOSE`.
8. `SUNK` means every cell occupied by that ship has been hit, so the ship is completely destroyed.
9. The match ends when one player remains alive.

Coordinates use a chess-style format:

- `A0` means row A, column 0
- `C7` means row C, column 7
- You can also type `ATTACK A5` during play for the standard 1v1 match

## Requirements

- Linux or WSL environment
- `g++` with C++17 support
- POSIX sockets support
- `make`
- A terminal for the server and one terminal per client

## Setup And Usage

### Build

```bash
make
```

### Run The Server

```bash
./server 4000
```

Optional: choose how many players should be grouped into each match.

```bash
./server 4000 3
```

### Run Clients

Open separate terminals for each player and run:

```bash
./client 127.0.0.1 4000 Alice
./client 127.0.0.1 4000 Bob
```

For a match with more than 2 players, use the target index shown by match order:

```bash
./client 127.0.0.1 4000 Carol
```

Then during your turn, enter attacks like:

```bash
ATTACK 1 B4
```

## Project Directory

```text
.
├── Makefile
├── README.md
├── client.cpp
├── server.cpp
├── client
├── server
├── include/
│   ├── BattleshipServer.h
│   ├── Board.h
│   ├── MatchSession.h
│   ├── Player.h
│   ├── Protocol.h
│   └── Ship.h
├── 1v1_Stage/
│   ├── battleship.exe
│   └── src/
│       ├── Board.h
│       ├── Game.h
│       ├── Player.h
│       ├── Ship.h
│       └── main.cpp
└── Socket Programming.docx
```

## Game Files In This Project

- `server.cpp` starts the network server and matchmaking system.
- `client.cpp` runs the terminal client and handles player input.
- `include/Protocol.h` defines the socket messaging helpers and protocol parsing.
- `include/BattleshipServer.h` coordinates client queues and match creation.
- `include/MatchSession.h` runs each active game session.
- `include/Board.h`, `include/Ship.h`, and `include/Player.h` implement the game model.

## Protocol Tokens

- `JOIN <name>`
- `READY`
- `MATCH_START players=<n>`
- `YOUR_TURN`
- `BOARD_BEGIN OWN`
- `BOARD_BEGIN OPPONENT`
- `ATTACK row col` for 1v1 matches
- `ATTACK target row col` for matches with 3 or more players
- `HIT`
- `MISS`
- `SUNK <ship>`
- `WIN`
- `LOSE`
- `SERVER_SHUTDOWN`

## Notes

- The project is designed for Linux/WSL.
- All gameplay logic, networking, and data structures are implemented in C++.
- The `1v1_Stage` folder keeps the earlier offline stage separate from the networked version.