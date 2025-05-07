# Music Server in C – BYOS Project

This project is a basic client-server music streaming system written in C, simulating a simplified Spotify-like experience using TCP sockets and multi-process server architecture.

## Features

### Server
- TCP-based server using `fork()` for concurrency
- Handles multiple clients simultaneously
- Supports:
  - `LIST`: send available songs
  - `GET <filename>`: stream a song
  - `LOGIN <username> <password>`: (in progress)
- Serves files from the `music/` directory
- Uses `users.txt` for login credentials

### Client
- Connects to server over TCP
- Sends commands (LIST, GET, LOGIN)
- Saves streamed songs to `downloads/` directory

## Project Structure

Musicserver/
│
├── bin/ # Compiled binaries
├── downloads/ # Where client saves songs
├── include/ # Header files
├── music/ # Songs stored on server
├── obj/ # Compiled object files
├── src/ # Source files
│ ├── simpleserver.c
│ ├── serverclient.c
│ ├── request_handler.c
│ ├── recieve_handler.c
│ └── network_utils.c
├── users.txt # Username:password list
├── Makefile
└── README.md
## How to Build & Run

```bash
make              # Build both server and client
./bin/simpleserver    # Start the server
./bin/serverclient    # Start a client