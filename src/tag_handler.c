#include "tag_handler.h"
#include "response_codes.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
 #include <dirent.h>
 #include <stdlib.h>



// ID3v1 tag sizes for fields
#define ID3V1_TAG_SIZE 128
#define ID3V1_TITLE_SIZE 30
#define ID3V1_ARTIST_SIZE 30
#define ID3V1_ALBUM_SIZE 30
#define ID3V1_YEAR_SIZE 4

// Load ID3v1 tag from an MP3 file safely by reading last 128 bytes
void read_id3v1_tag(int client_fd, const char *command) {
    const char *filename = command + 5;
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "music/%s", filename);
    int code = 0;
        
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        {
        code = ERR_FILE_NOT_FOUND;
        send(client_fd, &code, sizeof(code), 0);
        return;
    }
    } // File not found or couldn't be opened

    // Seek to last 128 bytes (location of ID3v1 tag)
    if (fseek(fp, -ID3V1_TAG_SIZE, SEEK_END) != 0) {
        fclose(fp);
        code = ERR_TAG_PARSE_FAIL;
        send(client_fd, &code, sizeof(code), 0);
        return;
    }

    // Read tag into buffer
    unsigned char buffer[ID3V1_TAG_SIZE];
    if (fread(buffer, 1, ID3V1_TAG_SIZE, fp) != ID3V1_TAG_SIZE) {
        fclose(fp);
        code = ERR_TAG_PARSE_FAIL;
        send(client_fd, &code, sizeof(code), 0);
        return;
    }
    fclose(fp); // Done reading the file

    // Check for the "TAG" identifier
    if (memcmp(buffer, "TAG", 3) != 0) {
        code = ERR_TAG_NOT_FOUND;
        send(client_fd, &code, sizeof(code), 0);
        return; // No ID3v1 tag found
    }
    // Extract and null-terminate each field safely
    struct ID3v1Tag tag;
    memset(&tag, 0, sizeof(tag));  // clear everything
    // Title (bytes 3–32)
    memcpy(tag.title,  buffer + 3,  ID3V1_TITLE_SIZE);
    tag.title[ID3V1_TITLE_SIZE] = '\0';
    // Artist (bytes 33–62)
    memcpy(tag.artist, buffer + 33, ID3V1_ARTIST_SIZE);
    tag.artist[ID3V1_ARTIST_SIZE] = '\0';
    // Album (bytes 63–92)
    memcpy(tag.album,  buffer + 63, ID3V1_ALBUM_SIZE);
    tag.album[ID3V1_ALBUM_SIZE] = '\0';
    // Year (bytes 93–96)
    memcpy(tag.year,   buffer + 93, ID3V1_YEAR_SIZE);
    tag.year[ID3V1_YEAR_SIZE] = '\0';
    // Read genre byte (last byte of 128-byte ID3v1 tag)
    tag.genre = buffer[127];

    code = OK;
    send(client_fd, &code, sizeof(code), 0);
    send(client_fd, &tag, sizeof(tag), 0);
    // Clear the tag structure to avoid memory leaks
    memset(&tag, 0, sizeof(tag));
}
void search_tag(int client_fd, const char *command) {
    // Expected format: "search <album/artist/year/genre> <value>"
    char tag_type[16], value[64];
    int code = 0;
    if (sscanf(command, "search %15s %63[^\n]", tag_type, value) != 2) { // Parse command to extract tag type and value
        code = ERR_PARSE;
        send(client_fd, &code, sizeof(code), 0);
        return;
    }

    // Open the music directory
    FILE *fp;
    struct ID3v1Tag tag;
    char filepath[256];
    int found = 0;

    // Check if the directory exists   
    DIR *dir = opendir("music");
    if (!dir) {
        code = ERR_FILE_NOT_FOUND;
        send(client_fd, &code, sizeof(code), 0);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Only process .mp3 files
        if (strstr(entry->d_name, ".mp3")) {
            snprintf(filepath, sizeof(filepath), "music/%.240s", entry->d_name); // 240 is a prefix to not to overflow the buffer filepath, even if d_name is max size. Warning was raised by compiler
            fp = fopen(filepath, "rb");
            if (!fp) continue; // Skip if file cannot be opened
            // Check if the file is at least 128 bytes long
            if (fseek(fp, -ID3V1_TAG_SIZE, SEEK_END) != 0) { //put pointer to the last 128 bytes and check if file is at least 128 bytes long
                fclose(fp);
                continue;
            }
            unsigned char buffer[ID3V1_TAG_SIZE];
            if (fread(buffer, 1, ID3V1_TAG_SIZE, fp) != ID3V1_TAG_SIZE) { // Read the last 128 bytes
                fclose(fp);
                continue;
            }
            fclose(fp);
            if (memcmp(buffer, "TAG", 3) != 0) continue;
            memset(&tag, 0, sizeof(tag));
            memcpy(tag.title,  buffer + 3,  ID3V1_TITLE_SIZE);
            tag.title[ID3V1_TITLE_SIZE] = '\0';
            memcpy(tag.artist, buffer + 33, ID3V1_ARTIST_SIZE);
            tag.artist[ID3V1_ARTIST_SIZE] = '\0';
            memcpy(tag.album,  buffer + 63,  ID3V1_ALBUM_SIZE);
            tag.album[ID3V1_ALBUM_SIZE] = '\0';
            memcpy(tag.year,   buffer + 93, ID3V1_YEAR_SIZE);
            tag.year[ID3V1_YEAR_SIZE] = '\0';
            tag.genre = buffer[127];
           
            int match = 0;
            if (strcasecmp(tag_type, "album") == 0 && strcasecmp(tag.album, value) == 0) match = 1;
            else if (strcasecmp(tag_type, "artist") == 0 && strcasecmp(tag.artist, value) == 0) match = 1;
            else if (strcasecmp(tag_type, "year") == 0 && strcasecmp(tag.year, value) == 0) match = 1;
            else if (strcasecmp(tag_type, "genre") == 0 && tag.genre == (unsigned char)atoi(value)) match = 1; // Compare genre as integer, atoi convert ASCII to Integer
            
            // If a match is found, send the song name
            if (match) {
                found = 1;
                // Send song name
                send(client_fd, &code, sizeof(code), 0);
                send(client_fd, entry->d_name, strlen(entry->d_name) + 1, 0);
            }
        }
    }
    closedir(dir);
    // If no matching tags were found, send an error code
    code = found ? OK : ERR_TAG_NOT_FOUND; // If found, send OK, otherwise send error code (condition ? value_if_true : value_if_false)
    send(client_fd, "END\n", 4, 0);
    
}



