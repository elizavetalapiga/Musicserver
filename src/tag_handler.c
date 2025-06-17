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
            else if (strcasecmp(tag_type, "genre") == 0 && (tag.genre == (unsigned char)atoi(value)||
          strcasecmp(get_genre_name(tag.genre), value) == 0)) match = 1; // Compare genre as integer, atoi convert ASCII to Integer
            
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

const char* get_genre_name(unsigned char genre) {
    switch (genre) {
        case 0:  return "Blues";
        case 1:  return "Classic Rock";
        case 4:  return "Disco";
        case 7:  return "Hip-Hop";
        case 13: return "Pop";
        case 17: return "Rock";
        case 22: return "Death Metal";
        case 25: return "Soundtrack";
        case 32: return "Classical";
        case 52: return "Electronic";
        default: return "Unknown Genre";
    }
}

 void handle_changetag(int client_fd, const char *command, const char *role){
    int respond = 0;
    char filename[128], tag_type[16], new_value[64];
    char filepath[256];
    struct ID3v1Tag tag;
    printf("[DEBUG] Role: '%s'\n", role);
    // Check for admin role
    if (strcmp(role, "admin") != 0) {
        respond = ERR_PERMISSION;
        send(client_fd, &respond, sizeof(respond), 0);
        return;
    }
    printf("[DEBUG] Entered handle_changetag()\n");
    // Parse command to extract filename, tag type and new value
    if (sscanf(command + 10, "%127s %15s %63[^\n]", filename, tag_type, new_value) != 3) {
        respond = ERR_PARSE;
        send(client_fd, &respond, sizeof(respond), 0);
        return;
    }
 printf("[DEBUG] Filename: %s, Tag Type: %s, New Value: %s\n", filename, tag_type, new_value);
    snprintf(filepath, sizeof(filepath), "music/%s", filename);
    
    FILE *fp = fopen(filepath, "rb+"); //open file for updating (read and write)
    if (!fp) {
        respond = ERR_FILE_NOT_FOUND;
        send(client_fd, &respond, sizeof(respond), 0);
        return; // File not found or couldn't be opened
    }
    printf("[DEBUG] Trying to open file: %s\n", filepath);
    // Seek to last 128 bytes (location of ID3v1 tag)
    if (fseek(fp, -ID3V1_TAG_SIZE, SEEK_END) != 0) {
        fclose(fp);
        respond = ERR_TAG_PARSE_FAIL;
        send(client_fd, &respond, sizeof(respond), 0);
        return;
    }

    // Read tag into buffer
    unsigned char buffer[ID3V1_TAG_SIZE];
    if (fread(buffer, 1, ID3V1_TAG_SIZE, fp) != ID3V1_TAG_SIZE) {
        fclose(fp);
        respond = ERR_TAG_PARSE_FAIL;
        send(client_fd, &respond, sizeof(respond), 0);
        return;
    }
    printf("[DEBUG] Read ID3v1 tag from file: %s\n", filepath);
    // Check for the "TAG" identifier
    if (memcmp(buffer, "TAG", 3) != 0) {
        fclose(fp);
        respond = ERR_TAG_NOT_FOUND;
        send(client_fd, &respond, sizeof(respond), 0);
        return; // No ID3v1 tag found
    }
    printf("[DEBUG] Found ID3v1 tag in file: %s\n", filepath);
    // Extract existing tag data
    memset(&tag, 0, sizeof(tag));
    memcpy(tag.title, buffer + 3, ID3V1_TITLE_SIZE);
    tag.title[ID3V1_TITLE_SIZE] = '\0';
    memcpy(tag.artist, buffer + 33, ID3V1_ARTIST_SIZE);
    tag.artist[ID3V1_ARTIST_SIZE] = '\0';
    memcpy(tag.album, buffer + 63, ID3V1_ALBUM_SIZE);
    tag.album[ID3V1_ALBUM_SIZE] = '\0';
    memcpy(tag.year, buffer + 93, ID3V1_YEAR_SIZE);
    tag.year[ID3V1_YEAR_SIZE] = '\0';
    tag.genre = buffer[127]; // Read genre byte (last byte of 128-byte ID3v1 tag)
   printf("[DEBUG] Current ID3v1 tag data:\n");
    // Update the tag based on the tag type
    if (strcasecmp(tag_type, "title") == 0) {
        strncpy(tag.title, new_value, ID3V1_TITLE_SIZE);
        tag.title[ID3V1_TITLE_SIZE] = '\0';
    } else if (strcasecmp(tag_type, "artist") == 0) {
        strncpy(tag.artist, new_value, ID3V1_ARTIST_SIZE);
        tag.artist[ID3V1_ARTIST_SIZE] = '\0';
    } else if (strcasecmp(tag_type, "album") == 0) {
        strncpy(tag.album, new_value, ID3V1_ALBUM_SIZE);
        tag.album[ID3V1_ALBUM_SIZE] = '\0';
    } else if (strcasecmp(tag_type, "year") == 0) {
        strncpy(tag.year, new_value, ID3V1_YEAR_SIZE);
        tag.year[ID3V1_YEAR_SIZE] = '\0';
    } else if (strcasecmp(tag_type, "genre") == 0) {
        tag.genre = (unsigned char)atoi(new_value); // Convert genre to integer. Admin can set genre only by number.        
    } else {
        fclose(fp);
        respond = ERR_TAG_NOT_FOUND;
        send(client_fd, &respond, sizeof(respond), 0);
        return; // Invalid tag type
    }
    printf("[DEBUG] Updated ID3v1 tag data:\n");
    printf("Title: %s\n", tag.title);
    printf("Artist: %s\n", tag.artist);
    printf("Album: %s\n", tag.album);
    printf("Year: %s\n", tag.year);       
    // Write updated tag back to file
    fseek(fp, -ID3V1_TAG_SIZE, SEEK_END); // Move pointer back to the start of the tag
    memset(buffer, 0, ID3V1_TAG_SIZE); // Clear buffer
    memcpy(buffer, "TAG", 3); // Set tag identifier
    memcpy(buffer + 3, tag.title, ID3V1_TITLE_SIZE);   
    memcpy(buffer + 33, tag.artist, ID3V1_ARTIST_SIZE);
    memcpy(buffer + 63, tag.album, ID3V1_ALBUM_SIZE);
    memcpy(buffer + 93, tag.year, ID3V1_YEAR_SIZE);
    buffer[127] = tag.genre; // Set genre byte
    if (fwrite(buffer, 1, ID3V1_TAG_SIZE, fp) != ID3V1_TAG_SIZE) {
        fclose(fp);
        respond = ERR_TAG_PARSE_FAIL;
        send(client_fd, &respond, sizeof(respond), 0);
        return; // Failed to write updated tag
    }
    printf("[DEBUG] Updated ID3v1 tag data:\n");
    
    fclose(fp); // Close the file
    respond = OK; // Success
    send(client_fd, &respond, sizeof(respond), 0); // Send success response
    send(client_fd, &tag, sizeof(tag), 0);      // Send updated tag data back to client
    memset(&tag, 0, sizeof(tag)); // Clear the tag structure to avoid memory leaks
 }
