# Music Server in C – BYOS Project

This project is a basic client-server music streaming system written in C, simulating a simplified Spotify-like experience using TCP sockets and multi-process server architecture.

## Functional Design Choices

- Users can:
For All Users:
- list – List all songs available on the server
- play <song_name> – Download and play a specific song
- logout – Log out from the current user session
- login <user> <pass> – Log into the account
- info <song_name> – Display metadata of the song (tags)
- search <album/artist/year/genre> <value> – Search for songs using metadata
- rate <song_name> – Rate the song (e.g., 1 to 5)
- avg <song_name> – View average rating of a song
- dlcount <song_name> – View how many times the song has been downloaded

- Admins can also:
- add <song_name> – Upload a new song to the server
- delete <song_name> – Delete a song from the server
- rename <song_name> <new_name> – Rename an existing song
- createuser <name> <password> <role> – Create a new user account (with role)
- changetag <song_name> <album/artist/year/genre> <value> – Modify a song's metadata

---

## Non-Functional Design Choices

- Fixed-size buffers instead of dynamic memory (`malloc`)
- Input length limits (e.g., 64 chars for usernames)
- Concurrent processing: Each client is handled in a separate process using fork().
- Crash recovery: Server re-scans the music directory on every command to rebuild its index.
- Safe fixed-size buffers: All input is size-limited and checked to avoid overflows.
- Text-based protocol: Commands are simple strings, easy to debug and test.
- Role-based access: Admin-only commands are restricted by user roles.
- Fresh metadata: Ratings, tags, and stats are reloaded from files, avoiding stale data.
- Graceful shutdown: Server and clients clean up resources and handle disconnects properly.
- Configurable port: Server port is set in a config file for flexibility.
- Structured error handling: Server uses numeric error codes for consistent communication.

---


## Data structures
#define MAX_SONGS 1000

// Represents ID3v1 metadata in MP3 files
struct ID3v1Tag {
    char tag[3];       
    char title[31];
    char artist[31];
    char album[31];
    char year[5];
    unsigned char genre;
};

// Indexed song entry
struct SongMetadata {
    char filename[256];     
    struct ID3v1Tag tag;    
};

// Global in-memory index
extern struct SongMetadata song_index[MAX_SONGS];
extern int song_count;


## Main Functions and Their Parameters

### Network setup (`network_utils.c`)

- int create_socket(): Creates a socket and returns its file descriptor.
- void handle_error(const char *message): Handles errors by printing a message and exiting the program (used for server or client configuration failure).
- void configure_server(struct sockaddr_in *server_addr, int port, const char *ip_address): Sets Up Server Address.
- void configure_client(struct sockaddr_in *server_addr, int port, const char *server_ip): Sets Up client Address.
- int load_config(char *ip_buffer, size_t ip_buf_size, int *port_out): loading the IP and port for configuration.

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