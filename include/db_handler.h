#ifndef DB_HANDLER_H
#define DB_HANDLER_H

#include <sqlite3.h>

int init_database(const char *filename);
int delete_song_db_entries(const char *song);
void close_database();

// Ratings
int rate_song(const char *song, const char *user, int rating);
float get_average_rating(const char *song);

// Downloads
int increment_download(const char *song);
int get_download_count(const char *song);



#endif
