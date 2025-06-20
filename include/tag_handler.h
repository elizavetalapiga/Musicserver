#ifndef TAG_HANDLER_H
#define TAG_HANDLER_H

#include <stdint.h>

#define MAX_SONGS 1000

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

extern struct SongMetadata song_index[MAX_SONGS]; // Declares the array of indexed songs defined elsewhere
extern int song_count; //Tracks how many entries are in song_index



int read_id3v1_tag(const char *filepath, struct ID3v1Tag *tag);
void search_tag(int client_fd, const char *command);
const char* get_genre_name(unsigned char genre);
void handle_changetag(int client_fd, const char *command, const char *role);
 void index_songs(const char *music_dir);
#endif 