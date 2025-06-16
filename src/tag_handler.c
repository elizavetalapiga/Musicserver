#include "tag_handler.h"
#include "response_codes.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>



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

