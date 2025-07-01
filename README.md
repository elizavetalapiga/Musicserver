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
- rate <song_name> <1-5> – Rate the song (e.g., 1 to 5)
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
- Incomplete uploads and downloads are detected and cleaned up on the server and client side.
- File transfer is validated using file size before and after transmission.

## Data structures
```c
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
extern struct SongMetadata *song_index;  
extern int song_count;
extern int song_capacity;
```

## Main Functions and Their Parameters

### Network setup (network_utils.c)
```c
- int create_socket() // Creates a socket and returns its file descriptor.
- void handle_error(const char *message) // Handles errors by printing a message and exiting the program (used for server or client configuration failure).
- void configure_server(struct sockaddr_in *server_addr, int port, const char *ip_address) // Sets Up Server Address.
- void configure_client(struct sockaddr_in *server_addr, int port, const char *server_ip) // Sets Up client Address.
- int load_config(char *ip_buffer, size_t ip_buf_size, int *port_out) // loading the IP and port for configuration.
```
### Tag and Song Management (tag_handler.h)
```c
- void init_song_index() // Initializes the dynamic song index.
- void free_song_index() // Frees the memory allocated for song index.
- void add_song_to_index(struct SongMetadata *new_song) // Adds a song entry to the in-memory index.
- int read_id3v1_tag(const char *filepath, struct ID3v1Tag *tag) // Reads tag info from an MP3 file.
- void search_tag(int client_fd, const char *command) // Searches tag fields based on client command.
- const char* get_genre_name(unsigned char genre) // Converts genre byte to a human-readable name.
- void handle_changetag(int client_fd, const char *command, const char *role) // Updates tag field for a song.
- void index_songs(const char *music_dir) // Rebuilds song index from the music directory.
```
### Database Operations (db_handler.h)
```c
- int init_database(const char *filename) // Opens SQLite DB and initializes tables.
- int delete_song_db_entries(const char *song) // Deletes all rating/download data for a song.
- void close_database() // Closes the SQLite DB connection.
- int rate_song(const char *song, const char *user, int rating) // Inserts/updates user rating.
- float get_average_rating(const char *song) // Computes average rating for a song.
- int increment_download(const char *song) // Increments download counter.
- int get_download_count(const char *song) // Returns total download count.
```
### Disk Space Checking (disk_space.h)
```c
- int check_disk_space(const char *path, long required_bytes, long *disk_space) // Verifies available disk space.
```
### Login/Auth (login.h and login_client.h)
```c
- int check_credentials(const char *username, const char *password, char *role_out) // Validates login info.
- int client_login(int sock_fd) // Handles login prompt and logic on client side.
```
### Request Handling – Server (request_handler.h)
```c
- void handle_cmd(int client_fd, const char *command, int *logged_in, char *role, char *username) // Parses and routes client commands.
- void handle_list(int client_fd) // Sends song list to client.
- void handle_get(int client_fd, const char *filename) // Sends requested song file.
- void handle_add(int client_fd, const char *command, const char *role) // Receives and stores a new song.
- void handle_delete(int client_fd, const char *command, const char *role) // Deletes a song from disk, DB, and index.
- void handle_rename(int client_fd, const char *command, const char *role) // Renames an existing song.
- void handle_newuser(int client_fd, const char *command, const char *role) // Adds a new user to credentials file.
- int handle_login(int client_fd, const char *command, char *role_out, char *username_out) // Server-side login logic.
- void handle_info(int client_fd, const char *filename) // Sends song metadata.
- void handle_rate(int client_fd, const char *args, const char *user) // Records user rating.
- void handle_avg(int client_fd, const char *args) // Sends average rating.
- void handle_dlcount(int client_fd, const char *args) // Sends download count.
- int song_exists(const char *songname) // Checks if a song is in the index and not deleted.
- int remove_song_from_index(const char *filename) // Removes a song from the in-memory index.
```
### Request Handling – Client (recieve_handler.h)
```c
- void handle_rcv(int sock_fd, const char *command) // Processes a generic response.
- void handle_rcv_list(int sock_fd) // Receives and displays song list.
- void handle_rcv_get(int sock_fd, const char *filename) // Receives and saves a song file.
- void handle_snd_add(int sock_fd, const char *filename) // Sends a new song file to server.
- void handle_rcv_delete(int sock_fd, const char *filename) // Handles server-side deletion response.
- void handle_rcv_newuser(int sock_fd) // Handles account creation.
- void handle_rcv_rename(int sock_fd) // Handles rename result.
- void handle_rcv_tag(int sock_fd) // Processes search tag result.
- void handle_search_response(int sock_fd) // Receives and prints search result.
- void handle_response(int response) // Parses numeric response codes.
- void handle_rcv_changetag(int sock_fd) // Handles tag update.
- void recv_and_print(int sock_fd) // Reads and prints a single-line message.
- void handle_rcv_rate(int sock_fd) // Handles rating request.
- void handle_rcv_avg(int sock_fd) // Receives and displays song rating.
- void handle_rcv_dlcount(int sock_fd) // Receives and displays download count.
- void handle_play(const char *filename) // Plays a song using system player.
- void cleanup_cache() // Removes temporary downloaded files.
```
### Cache Handler (cache_handler.h)
```c
- int check_cache(const char *filename) // Checks if a file exists in local cache.
- void cleanup_cache() // Removes cached files from temp dir.
```
## Known Problems
- No password hashing or encryption is used (plain text credentials).
- No full validation of metadata fields (e.g., genre number may be out of range).
- The file system is trusted—no integrity check for song file corruption.
- All commands are sent as plaintext over TCP; no encryption.
- SQLite database is accessed without mutexes; not thread-safe under heavy concurrency. However, each child process opens its own SQLite connection to avoid shared state issues.
- No full validation of metadata fields (e.g., genre number may be out of range).
- There is no full retry mechanism for failed transfers, but incomplete uploads can trigger a resend on the client side.
- Songs deleted from disk (manually, outside of the program) but not removed from memory can cause inconsistency if remove_song_from_index() is not called.

## Test cases

### Login
| Test Case        | Description                      | Expected Result          |
|------------------|----------------------------------|--------------------------|
| Valid login      | Correct username/password        | Login successful         |
| Invalid login    | Wrong password                   | Login failed             |
| Nonexistent user | Username not in file             | Login failed             |
| Admin login      | Admin credentials                | Login as admin           |

### Song Listing
| Test Case      | Description              | Expected Result           |
|----------------|--------------------------|---------------------------|
| List songs     | Songs in directory       | Full list shown           |
| No songs       | Empty music directory    | No songs shown            |

### Upload / Add Song
| Test Case         | Description                         | Expected Result                      |
|-------------------|-------------------------------------|--------------------------------------|
| Upload as admin   | Admin uploads song                 | File added, index updated            |
| Upload as user    | Normal user attempts upload         | Permission denied                    |
| Duplicate upload  | File already exists                | ERR_FILE_EXISTS                      |
| Interrupted upload| Simulate client/network drop        | Server deletes partial file, returns ERR |
| Disk full         | Upload exceeds available space      | ERR_DISK_IS_FULL                     |
| No ID3 tag        | File without ID3v1 tag             | Stored with empty metadata           |

### Download / Play
| Test Case          | Description                 | Expected Result           |
|---------------------|-----------------------------|----------------------------|
| Download song       | Valid file request         | File downloaded            |
| Download missing    | File not in index          | ERR_FILE_NOT_FOUND         |
| Play song           | Use system audio player    | Song plays                 |

### Metadata
| Test Case     | Description               | Expected Result            |
|----------------|---------------------------|-----------------------------|
| Info command   | View song metadata        | Title/artist/year shown     |
| Search by tag  | Search by genre/artist    | Matching songs returned     |
| Invalid field  | Search by wrong tag type  | ERR_TAG_PARSE_FAIL          |

### Ratings and Stats
| Test Case         | Description                      | Expected Result                |
|-------------------|----------------------------------|-------------------------------|
| Rate song         | User gives rating (1–5)          | Rating saved in DB            |
| Re-rate song      | User changes rating              | Previous rating updated       |
| Invalid rating    | Out of bounds rating             | ERR_PARSE or ignored          |
| Get average       | Fetch average rating             | Float value shown             |
| No ratings yet    | No ratings recorded              | "No ratings yet" message      |
| Download count    | Song downloaded many times       | Count matches total           |

### Admin Functions
| Test Case          | Description                    | Expected Result               |
|--------------------|--------------------------------|-------------------------------|
| Create user        | Admin creates new user         | User added to users.txt       |
| Rename song        | Change song filename           | File and index updated        |
| Delete song        | Remove song from server        | File + DB + index cleaned     |
| Delete missing song| Try to delete nonexistent song | ERR_FILE_NOT_FOUND            |
| Change metadata    | Modify album/genre/etc.        | Metadata updated in memory    |

### Error Handling
| Test Case           | Description                        | Expected Result                  |
|---------------------|------------------------------------|----------------------------------|
| Bad DB file         | Corrupted SQLite DB                | Startup/init failure             |
| DB contention       | Multiple writers (forked clients)  | Busy timeout retries             |
| Client disconnect   | During upload/download             | Cleanup + ERR sent               |
| Partial file        | Upload interrupted mid-transfer    | File deleted, resend triggered   |
| Server shutdown     | SIGINT or crash                    | All resources released safely    |


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
