#ifndef TAG_HANDLER_H
#define TAG_HANDLER_H

#include <stdint.h>



struct ID3v1Tag {
    char tag[3];      // "TAG"
    char title[31];
    char artist[31];
    char album[31];
    char year[5];
    unsigned char genre;
};
struct SongMetadata {
    char filename[256];
    struct ID3v1Tag tag;  
};

extern struct SongMetadata *song_index;  // Declares that indexed songs defined elsewhere
extern int song_count; //Tracks how many entries are in song_index
extern int song_capacity;
//Initialization and cleanup
void init_song_index();
void free_song_index();
void add_song_to_index(struct SongMetadata *new_song);

int read_id3v1_tag(const char *filepath, struct ID3v1Tag *tag);
void search_tag(int client_fd, const char *command);
const char* get_genre_name(unsigned char genre);
void handle_changetag(int client_fd, const char *command, const char *role);
int changetag_song_in_indexes(const char *filename, const struct ID3v1Tag *new_tag);
void index_songs(const char *music_dir);
int remove_song_from_index(const char *filename);
int rename_song_in_indexes(const char *old_filename, const char *new_filename);
#endif 