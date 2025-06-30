#include "db_handler.h"
#include <stdio.h>
#define DB_PATH "music.db"

static sqlite3 *db = NULL;// A pointer to the SQLite connection that all functions will share

// Initialize the database connection. This function is called once at server startup
int init_database(const char *filename) {
    if (sqlite3_open(filename, &db) != SQLITE_OK) { // Open the database file
        fprintf(stderr, "Failed to open database: %s\n", sqlite3_errmsg(db)); // Print error message if opening fails on server side
        return 0;
    }

    // wait when the DB is locked, instead of instantly failing
    sqlite3_busy_timeout(db, 3000); // waits up to 3s

    return 1;

}
// Closes the database when the server exits. Prevents memory leaks and file locking issues.
void close_database() {
    if (db) sqlite3_close(db);
}

// RATING FUNCTIONS
int rate_song(const char *song, const char *user, int rating) {
    const char *sql =
        "INSERT INTO ratings (song, user, rating) VALUES (?, ?, ?) " //If (song, user) exists then update the rating. Else insert a new row
        "ON CONFLICT(song, user) DO UPDATE SET rating=excluded.rating;"; //Tells SQLite what to do if the same (song, user) already exists (because it's a primary key)
    
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0; //Compiles SQL into an executable statement for further execution
    // Replaces ? placeholders with actual values. 
    sqlite3_bind_text(stmt, 1, song, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, user, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, rating);

    int rc = sqlite3_step(stmt); // Executes the prepared statement. Returns SQLITE_DONE if successful.
    sqlite3_finalize(stmt); //Frees resources tied to stmt
    return rc == SQLITE_DONE; //If rc == SQLITE_DONE, return 1 (true)
}

float get_average_rating(const char *song) {
    printf("[DEBUG] Getting average rating for song: %s\n", song); //Debugging output to show which song is being queried
    const char *sql = "SELECT AVG(rating) FROM ratings WHERE song = ?;"; //Using AVG(rating) to compute the average score for a song
    //Prepares the SQL query and binds the song name.
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1; 

    sqlite3_bind_text(stmt, 1, song, -1, SQLITE_STATIC);

    float avg = -1.0; //if the song has no ratings, return -1.0
    if (sqlite3_step(stmt) == SQLITE_ROW) {  // Executes the query and checks if a row is returned
        avg = sqlite3_column_double(stmt, 0); // Retrieves the average rating from the result set
    }

    sqlite3_finalize(stmt);
    printf("[DEBUG] Getting average rating for song: %f\n", avg); 
    return avg;
}

// DOWNLOAD FUNCTIONS
int increment_download(const char *song) {
    printf("[DEBUG] Incrementing download count for song: %s\n", song); //Debugging output to show which song is being incremented
    const char *sql =
        "INSERT INTO downloads (song, count) VALUES (?, 1) " // Inserts a new row with count = 1 if the song does not exist
        "ON CONFLICT(song) DO UPDATE SET count = count + 1;"; // If the song already exists, increments the count by 1

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return 0;

    sqlite3_bind_text(stmt, 1, song, -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

int get_download_count(const char *song) {
    const char *sql = "SELECT count FROM downloads WHERE song = ?;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, song, -1, SQLITE_STATIC);
printf("[DEBUG] Getting download count for song: %s\n", song); //Debugging output to show which song is being queried
    int count = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return count; // Returns the download count for the specified song, or -1 if not found
}

// Deletes all database entries related to a song, including ratings and download counts
int delete_song_db_entries(const char *song) {
    const char *sql[] = {
        "DELETE FROM ratings WHERE song = ?;",
        "DELETE FROM downloads WHERE song = ?;"        
    };

    sqlite3_stmt *stmt;
    for (size_t i = 0; i < sizeof(sql) / sizeof(sql[0]); ++i) {
        if (sqlite3_prepare_v2(db, sql[i], -1, &stmt, NULL) != SQLITE_OK) {
            fprintf(stderr, "Failed to prepare delete: %s\n", sqlite3_errmsg(db));
            return 0;
        }

        sqlite3_bind_text(stmt, 1, song, -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            fprintf(stderr, "Failed to execute delete for: %s\n", song);
            sqlite3_finalize(stmt);
            return 0;
        }

        sqlite3_finalize(stmt);
    }

    return 1;  // All deletes succeeded
}