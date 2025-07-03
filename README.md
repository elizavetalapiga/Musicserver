# Music Server in C – BYOS Project

This project is a basic client-server music streaming system written in C, simulating a simplified Spotify-like experience using TCP sockets and multi-process server architecture.


## Instructions for Building and Running

### Prerequisites
- GCC compiler (or any C compiler)
- SQLite3 development libraries (libsqlite3-dev on Debian/Ubuntu)
- Make

### Project structure
```bash
Musicserver/
├── bin/              # Compiled binaries
├── client_music/     # Where the client stores downloaded songs
├── include/          # Header files
├── music/            # Server-side song files
├── obj/              # Compiled object files
├── src/              # Source code
├── credentials.txt   # User credentials (username:password:role)
├── config.txt        # Config file for connection (IP, port)
├── music.db          # SQLite database for ratings/downloads
├── Makefile          # Build configuration
└── README.md
```

### How to Build & Run

```bash
make              # Build both server and client
./bin/simpleserver    # Start the server
./bin/serverclient    # Start a client 
```

### Playing Music (Client)
The client attempts to play downloaded songs using a system MP3 player.
If no player is installed, we recommend installing one of the following:
- On Debian/Ubuntu: sudo apt install ffmpeg (includes ffplay)

- On Fedora: sudo dnf install ffmpeg

- On Arch: sudo pacman -S ffmpeg
By default, this client uses ffplay for playback. Make sure it is in your system path.

### Configuration
The server reads the port and IP from a config file.
Make sure the music/ folder exists and is writable by the server.


## Author

- **Elizaveta Lapiga**  
- Sapienza University of Rome, Computer Sytems and Programming  
- Email or GitHub: [elizavetalapiga](https://github.com/elizavetalapiga)
