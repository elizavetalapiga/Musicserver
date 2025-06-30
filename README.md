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


- Concurrent processing: Each client is handled in a separate process using fork().
- Crash recovery: Server re-scans the music directory on every command to rebuild its index.
- Text-based protocol: Commands are simple strings, easy to debug and test.
- Role-based access: Admin-only commands are restricted by user roles.
- Fresh metadata: Ratings, tags, and stats are reloaded from files, avoiding stale data.
- Graceful shutdown: Server and clients clean up resources and handle disconnects properly.
- Configurable port: Server port is set in a config file for flexibility.
- Structured error handling: Server uses numeric error codes for consistent communication.
- Fixed-size tag fields (ID3v1 format).
- Efficient memory usage via dynamic allocation in index process.
- SQLite database access protected with sqlite3_busy_timeout() to avoid lock collisions.
- Error handling on all file, DB, and socket operations.
- No threading used; child processes manage isolated DB sessions.
- Memory cleanup handled with proper free() and file closure.

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

### Network setup (network_utils.c)

- int create_socket(): Creates a socket and returns its file descriptor.
- void handle_error(const char *message): Handles errors by printing a message and exiting the program (used for server or client configuration failure).
- void configure_server(struct sockaddr_in *server_addr, int port, const char *ip_address): Sets Up Server Address.
- void configure_client(struct sockaddr_in *server_addr, int port, const char *server_ip): Sets Up client Address.
- int load_config(char *ip_buffer, size_t ip_buf_size, int *port_out): loading the IP and port for configuration.

### Tag and Song Management (tag_handler.h)

- void init_song_index(): Initializes the dynamic song index.
- void free_song_index(): Frees the memory allocated for song index.
- void add_song_to_index(struct SongMetadata *new_song): Adds a song entry to the in-memory index.
- int read_id3v1_tag(const char *filepath, struct ID3v1Tag *tag): Reads tag info from an MP3 file.
- void search_tag(int client_fd, const char *command): Searches tag fields based on client command.
- const char* get_genre_name(unsigned char genre): Converts genre byte to a human-readable name.
- void handle_changetag(int client_fd, const char *command, const char *role): Updates tag field for a song.
- void index_songs(const char *music_dir): Rebuilds song index from the music directory.

### Database Operations (db_handler.h)

- int init_database(const char *filename): Opens SQLite DB and initializes tables.
- int delete_song_db_entries(const char *song): Deletes all rating/download data for a song.
- void close_database(): Closes the SQLite DB connection.
- int rate_song(const char *song, const char *user, int rating): Inserts/updates user rating.
- float get_average_rating(const char *song): Computes average rating for a song.
- int increment_download(const char *song): Increments download counter.
- int get_download_count(const char *song): Returns total download count.

### Disk Space Checking (disk_space.h)
- int check_disk_space(const char *path, long required_bytes, long *disk_space): Verifies available disk space.

### Login/Auth (login.h and login_client.h)

- int check_credentials(const char *username, const char *password, char *role_out): Validates login info.
- int client_login(int sock_fd): Handles login prompt and logic on client side.

### Request Handling – Server (request_handler.h)

- void handle_cmd(int client_fd, const char *command, int *logged_in, char *role, char *username): Parses and routes client commands.
- void handle_list(int client_fd): Sends song list to client.
- void handle_get(int client_fd, const char *filename): Sends requested song file.
- void handle_add(int client_fd, const char *command, const char *role): Receives and stores a new song.
- void handle_delete(int client_fd, const char *command, const char *role): Deletes a song from disk, DB, and index.
- void handle_rename(int client_fd, const char *command, const char *role): Renames an existing song.
- void handle_newuser(int client_fd, const char *command, const char *role): Adds a new user to credentials file.
- int handle_login(int client_fd, const char *command, char *role_out, char *username_out): Server-side login logic.
- void handle_info(int client_fd, const char *filename): Sends song metadata.
- void handle_rate(int client_fd, const char *args, const char *user): Records user rating.
- void handle_avg(int client_fd, const char *args): Sends average rating.
- void handle_dlcount(int client_fd, const char *args): Sends download count.
- int song_exists(const char *songname): Checks if a song is in the index and not deleted.
- int remove_song_from_index(const char *filename): Removes a song from the in-memory index.

### Request Handling – Client (recieve_handler.h)

- void handle_rcv(int sock_fd, const char *command): Processes a generic response.
- void handle_rcv_list(int sock_fd): Receives and displays song list.
- void handle_rcv_get(int sock_fd, const char *filename): Receives and saves a song file.
- void handle_snd_add(int sock_fd, const char *filename): Sends a new song file to server.
- void handle_rcv_delete(int sock_fd, const char *filename): Handles server-side deletion response.
- void handle_rcv_newuser(int sock_fd): Handles account creation.
- void handle_rcv_rename(int sock_fd): Handles rename result.
- void handle_rcv_tag(int sock_fd): Processes search tag result.
- void handle_search_response(int sock_fd): Receives and prints search result.
- void handle_response(int response): Parses numeric response codes.
- void handle_rcv_changetag(int sock_fd): Handles tag update.
- void recv_and_print(int sock_fd): Reads and prints a single-line message.
- void handle_rcv_rate(int sock_fd): Handles rating request.
- void handle_rcv_avg(int sock_fd): Receives and displays song rating.
- void handle_rcv_dlcount(int sock_fd): Receives and displays download count.
- void handle_play(const char *filename): Plays a song using system player.
- void cleanup_cache(): Removes temporary downloaded files.

### Cache Handler (cache_handler.h)

- int check_cache(const char *filename): Checks if a file exists in local cache.
- void cleanup_cache(): Removes cached files from temp dir.

## Known Problems
- No password hashing or encryption is used (plain text credentials).
- No full validation of metadata fields (e.g., genre number may be out of range).
- The file system is trusted—no integrity check for song file corruption.
- All commands are sent as plaintext over TCP; no encryption.

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