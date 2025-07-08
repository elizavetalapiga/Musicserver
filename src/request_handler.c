#include "request_handler.h"
#include "network_utils.h"
#include "response_codes.h"
#include "login.h"
#include "tag_handler.h"
#include "db_handler.h"
#include "disk_space.h"
#include <string.h>
#include <stdio.h>

#include <dirent.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <unistd.h>
#include <arpa/inet.h>



void handle_cmd(int client_fd, const char *command, int *logged_in, char *role, char *username){
      // Always allow LOGIN
      if (strncasecmp(command, "LOGIN ", 6) == 0) {
        *logged_in = handle_login(client_fd, command, role, username); // returns 1 if successful
        return;
    }

    // Always allow LOGOUT
    if (strcasecmp(command, "LOGOUT") == 0) {
        *logged_in = 0;
        int respond = OK;
        send(client_fd, &respond, sizeof(respond), 0);     
        return;
    }

    // Block all other commands if not logged in
    if (!*logged_in) {
      int respond = ERR_PERMISSION;
      send(client_fd, &respond, sizeof(respond), 0);
      return;
    }

  //other commands
   if (strcasecmp(command, "LIST") == 0) {
      handle_list(client_fd);
      return;
      }
    else if (strncasecmp(command, "PLAY ", 5) == 0) {
      handle_get(client_fd, command + 5);
      return;
      }
    else if (strncasecmp(command, "ADD ", 4) == 0) {
      handle_add(client_fd, command, role);
      return;
      }
    else if (strncasecmp(command, "DELETE ", 7) == 0) {
      handle_delete(client_fd, command, role);
      }
    else if (strncasecmp(command, "RENAME ", 7) == 0) {
      handle_rename(client_fd, command, role);
      return;
      } 
    else if (strncasecmp(command, "CREATEUSER ", 11) == 0) {
      handle_newuser(client_fd, command, role);
      return;
      }
    else if (strncasecmp(command, "INFO ", 5) == 0) {
      handle_info(client_fd, command + 5);
      return;
      }
    else if (strncasecmp(command, "SEARCH ", 7) == 0) {
      search_tag(client_fd, command);
      return;
      }
    else if (strncasecmp(command, "CHANGETAG ", 10) == 0) {
      handle_changetag(client_fd, command, role);
      return;
      }
    else if (strncasecmp(command, "RATE ", 5) == 0) {
      handle_rate(client_fd, command + 5, username);
      return;
      }
    else if (strncasecmp(command, "AVG ", 4) == 0) {
      handle_avg(client_fd, command + 4);
      return;
      }
    else if (strncasecmp(command, "DLCOUNT ", 8) == 0) {
      handle_dlcount(client_fd, command + 8);
      return;
      }
    else {
      send(client_fd, "ERROR: Unknown command\n", 24, 0);
  }
}

void handle_list(int client_fd){
  char buffer [512] = {0};  
  int respond = OK;

  send(client_fd, &respond, sizeof(respond), 0);

  for (int i = 0; i < song_count; i++) {
      snprintf(buffer, sizeof(buffer), "%s\n", song_index[i].filename);      
      send(client_fd, buffer, strlen(buffer), 0);
  }
  // Send end marker
  send(client_fd, "END\n", 4, 0);
  
}

void handle_get (int client_fd, const char *filename) {
  char buffsndr[1024] = {0};
  size_t reads_bytes;
  char local_filename[256] = {0};
  char path[512] = {0};
  int respond = 0;
  long filesize = 0;
  // Trim newline, since const was passed we cannot change filename directly
  strncpy(local_filename, filename, sizeof(local_filename)-1); //-1 to leave the room for \0
  local_filename[sizeof(local_filename)-1] = '\0';  // safety null(if the filename bigger then buffer anyway it will end with \0)
  local_filename[strcspn(local_filename, "\n")] = '\0';

  //building the path
  memset(path, 0, sizeof(path));
  snprintf(path, sizeof(path), "music/%s", local_filename);
  printf("Trying to open file: %s\n", path);

  // Open the file descriptor
  int fd = open(path, O_RDONLY);
  if (fd < 0) {      
      respond = ERR_FILE_NOT_FOUND;
      send(client_fd, &respond, sizeof(respond), 0);      
      return;
  }

    // Appling the shared lock to allow safe concurrent reading
  if (flock(fd, LOCK_SH) < 0) {
      close(fd);
      respond = ERR_LOCK_FAILED;
      send(client_fd, &respond, sizeof(respond), 0);
      return;
  }

  //checking if the file can be open. RB - Read in Binary Mode, required for mp3
  FILE *file =  fdopen(fd, "rb");
  if (!file) {
    close(fd);
    respond = ERR_FILE_OPEN_FAIL;
    send(client_fd, &respond, sizeof(respond), 0); 
    return;
  }

  respond = OK;
  send(client_fd, &respond, sizeof(respond), 0);

  //filesize collection ans sending in order to stop the loop on client side
  fseek(file, 0, SEEK_END);
  filesize = ftell(file);
  rewind(file); //reset pointer to the begining of file
  send(client_fd, &filesize, sizeof(filesize), 0);


  //reciving respond
  recv(client_fd, &respond, sizeof(respond), 0);
  if (respond != OK) {
  perror("Error occured");
  return;
  }
  
  // Sending chunks + Error handeling
  while ((reads_bytes = fread(buffsndr, 1, sizeof(buffsndr), file)) > 0){
    if ((send(client_fd, buffsndr, reads_bytes, 0)) == -1){
    fclose(file); // releases lock and closes fd
    perror("Send failed");
    return;
    }
    
  }
  //check
  printf("File was sent: %s\n", path);
  fclose(file);
  increment_download(local_filename);
}



int handle_login(int client_fd, const char *command, char *role_out, char *username_out) {
  char username[64] = {0}, password[64] = {0};
  char role[64] = {0};
  int respond = 0;
  

  // Try to parse: LOGIN <username> <password>
  if (sscanf(command + 6, "%63s %63s", username, password) != 2) {
      respond = ERR_PARSE;
      send(client_fd, &respond, sizeof(respond), 0);
      return 0;
  }
 
  if (check_credentials(username, password, role)) {
      strcpy(role_out, role); // passing role back to caller
      strcpy(username_out, username); 
      respond = OK;
      send(client_fd, &respond, sizeof(respond), 0);
      return 1;
  } else {
      respond = ERR_PERMISSION;
      send(client_fd, &respond, sizeof(respond), 0);
      return 0;
  }
}

void handle_add(int client_fd, const char *command, const char *role) {
  int respond = 0;
  int fd;
  char filename[256] = {0};
  char filepath[512] = {0};
  long filesize = 0;
  char buffer[1024] = {0};
  long received = 0;
  int chunk;
  long disk_space = 0;


  //check for admin role
  if (strcmp(role, "admin") != 0) {
      respond = ERR_PERMISSION;
      send(client_fd, &respond, sizeof(respond), 0);
      return;
  }

  //check the song name absence and parsing the songname in variable
  if (sscanf(command + 4, "%255s", filename) != 1) {
      respond = ERR_FILE_NOT_FOUND;
      send(client_fd, &respond, sizeof(respond), 0);
      return;
  }

    //crafting the filepath
  snprintf(filepath, sizeof(filepath), "music/%s", filename);


  if (access(filepath, F_OK) == 0) {// check if file exsists
      respond = ERR_FILE_EXISTS;
      send(client_fd, &respond, sizeof(respond), 0);
      return;
  } else {
      respond = OK; // file does not exist, we can proceed
      send(client_fd, &respond, sizeof(respond), 0);
      }

    
    //reciving the filesize. Check for failed read or connection closed, and for invalid data
  if (recv(client_fd, &filesize, sizeof(filesize), 0) <= 0 || filesize <= 0) {
      respond = ERR_GENERIC;
      send(client_fd, &respond, sizeof(respond), 0);
      return;
  }
  if (check_disk_space("music", filesize, &disk_space) == 0){
    respond = ERR_DISK_IS_FULL;
    send(client_fd, &respond, sizeof(respond), 0);
    return;
  }



  // Open the file for writing (truncate if exists, create if not)
  fd = open(filepath, O_WRONLY | O_CREAT, 0666); //O_CREAT - If the file doesn’t exist, create it. 0666 - permission bits
  if (fd < 0) {
      respond = ERR_FILE_OPEN_FAIL;
      send(client_fd, &respond, sizeof(respond), 0);
      return;
  }

 //Lock the file exclusively
  if (flock(fd, LOCK_EX) < 0) {
      respond = ERR_GENERIC;
      close(fd);
      send(client_fd, &respond, sizeof(respond), 0);
      return;
  }

  //creating file for writing
  FILE *file = fdopen(fd, "wb");
  if (!file) {
      respond = ERR_FILE_OPEN_FAIL;
      close(fd);
      send(client_fd, &respond, sizeof(respond), 0);
      return;
  }


 //recieving file
  while (received < filesize) {
      chunk = recv(client_fd, buffer, sizeof(buffer), 0);
      if (chunk <= 0) {// check for failed read or connection closed
        perror("Recieved failed");
        fclose(file);
        remove(filepath);
        respond = ERR_INCOMPLETE_TRANSFER;
        send(client_fd, &respond, sizeof(respond), 0);
        return;        
      }
      fwrite(buffer, 1, chunk, file);
      received += chunk;
  }
  fclose(file); // unlocks + closes


send(client_fd, &respond, sizeof(respond), 0);

}


void handle_delete(int client_fd, const char *command, const char *role) {
  int respond = 0;
  char filename[256] = {0};
  char filepath[512] = {0};
  
  //check for admin role
  if (strcmp(role, "admin") != 0) {
      respond = ERR_PERMISSION;
      send(client_fd, &respond, sizeof(respond), 0);
      return;
  }

  //check the song name absence and parsing the songname in variable
  if (sscanf(command + 7, "%255s", filename) != 1) {
      respond = 0;
      send(client_fd, &respond, sizeof(respond), 0);
      return;
  }

  //crafting the filepath
  snprintf(filepath, sizeof(filepath), "music/%s", filename);

  //removing file, sending the respond
  if (remove(filepath) == 0) {
    respond = OK;
    send(client_fd, &respond, sizeof(respond), 0);
    if (!delete_song_db_entries(filename)) {
        respond = ERR_GENERIC;
        send(client_fd, &respond, sizeof(respond), 0);
    }

    if (!remove_song_from_index(filename)) {
        respond = ERR_GENERIC;
        send(client_fd, &respond, sizeof(respond), 0);
    }
  } else {
    respond = ERR_FILE_NOT_FOUND;
    send(client_fd, &respond, sizeof(respond), 0);
  }
  return;
}

void handle_rename(int client_fd, const char *command, const char *role) {
int respond;
int fd;
char old_name [256] = {0}, new_name [256] = {0}, old_path [512] = {0}, new_path [512] = {0};

//check for admin role
if (strcmp(role, "admin") != 0) {
    respond = ERR_PERMISSION;
    send(client_fd, &respond, sizeof(respond), 0);
    return;
}

//check the song name absence and parsing the songname in variable
if (sscanf(command + 7, "%255s %255s", old_name, new_name) != 2) {
    respond = ERR_GENERIC;
    send(client_fd, &respond, sizeof(respond), 0);
    return;
}

snprintf(old_path, sizeof(old_path), "music/%s", old_name);
snprintf(new_path, sizeof(new_path), "music/%s", new_name);

// Open the old file for locking
fd = open(old_path, O_RDWR);
if (fd < 0) {
  respond = ERR_FILE_OPEN_FAIL;
  send(client_fd, &respond, sizeof(respond), 0);
  return;
}

// Lock the file exclusively to block readers/writers
if (flock(fd, LOCK_EX) < 0) {
  close(fd);
  respond = ERR_GENERIC;
  send(client_fd, &respond, sizeof(respond), 0);
  return;
}


if (rename(old_path, new_path) == 0) {
    rename_song_in_indexes(old_name, new_name);  // update index too
    respond = OK;
} else {
    respond = ERR_GENERIC;
}
send(client_fd, &respond, sizeof(respond), 0);
close(fd);  // releases lock

}


void handle_newuser(int client_fd, const char *command, const char *role){
int respond;
FILE *file;
char buffer [128] = {0}, existing_user [64] = {0}, username [64] = {0}, password [64] = {0}, new_role [64] = {0};
//check for admin role
if (strcmp(role, "admin") != 0) {
    respond = ERR_PERMISSION;
    send(client_fd, &respond, sizeof(respond), 0);
    return;
}

//check the credentials absence and parsing it in variables
if (sscanf(command + 11, "%63s %63s %15s", username, password, new_role) != 3) {
  respond = ERR_GENERIC;
  send(client_fd, &respond, sizeof(respond), 0);
  return;
}
new_role[strcspn(new_role, "\n")] = '\0';
//check id the role is valid
if (strncasecmp(new_role, "admin", 5) != 0 && strncasecmp(new_role, "user", 4)!= 0){
  respond = ERR_INVALID_ROLE;
  send(client_fd, &respond, sizeof(respond), 0);
  return;
}

//check if user already exists
if ((file=fopen("credentials.txt", "r")) == NULL){
  respond = ERR_FILE_OPEN_FAIL;
  send(client_fd, &respond, sizeof(respond), 0);
  return;
}

while ((fgets(buffer, sizeof(buffer), file)) != NULL){
  if (sscanf(buffer, "%63[^:]", existing_user) == 1 && strcmp(username, existing_user) == 0) {
    fclose(file);
    respond = ERR_FILE_OPEN_FAIL;
    send(client_fd, &respond, sizeof(respond), 0);
    return;
  }
}

fclose(file);

//adding user. "a": Open for appending.
file = fopen("credentials.txt", "a");
if (file == NULL){
  respond = ERR_FILE_OPEN_FAIL;
  send(client_fd, &respond, sizeof(respond), 0);
  return;
}
fprintf(file, "%s:%s:%s\n", username, password, new_role);
fclose(file);
respond = OK;
send(client_fd, &respond, sizeof(respond), 0);

}

void handle_info(int client_fd, const char *filename) {
    int found = 0;
    int response;

    for (int i = 0; i < song_count; i++) {
        if (strcmp(song_index[i].filename, filename) == 0) {// Check if song matches
            // Prepare the tag information to send
            struct ID3v1Tag *tag = &song_index[i].tag; //put matched song in tag structure
            response = OK; // Set response code to OK
            send(client_fd, &response, sizeof(response), 0);
            char buffer[512];  // adjust size as needed
            snprintf(buffer, sizeof(buffer),
              "Title: %s\n"
              "Artist: %s\n"
              "Album: %s\n"
              "Year: %s\n"
              "Genre: %d (%s)\n",
              tag->title,
              tag->artist,
              tag->album,
              tag->year,
              tag->genre,
              get_genre_name(tag->genre));
            send(client_fd, buffer, strlen(buffer) + 1, 0);
            found = 1;
            break;
        }
    }

    if (!found) {
        response = ERR_FILE_NOT_FOUND;
        send(client_fd, &response, sizeof(response), 0);
    }
}

void handle_rate(int client_fd, const char *args, const char *user) {
    char songname[256] = {0};
    int rating;
    
    
  
    if (sscanf(args, "%255s %d", songname, &rating) != 2 || rating < 1 || rating > 5) { //check if comand is valid
        dprintf(client_fd, "ERR Invalid usage. Use: RATE <song> <1-5>\n");
        return;
    }

    if (!song_exists(songname)) {
    dprintf(client_fd, "ERR Song '%s' does not exist.\n", songname);
    return;
    }

    if (rate_song(songname, user, rating)) {
        dprintf(client_fd, "OK Rating saved for '%s'.\n", songname);
    } else {
        dprintf(client_fd, "ERR Failed to save rating.\n");
    }
}


void handle_avg(int client_fd, const char *args) {
    char songname[256] = {0};

    if (sscanf(args, "%255s", songname) != 1) {
        dprintf(client_fd, "ERR Usage: AVG <songname>\n");
        return;
    }

    if (!song_exists(songname)) {
    dprintf(client_fd, "ERR Song '%s' does not exist.\n", songname);
    return;
      }

    float avg = get_average_rating(songname);
    if (avg < 0) {
        dprintf(client_fd, "No ratings yet for '%s'.\n", songname);
    } else {
        dprintf(client_fd, "Average rating for '%s' is %.2f\n", songname, avg); //2 decimal places
    }
}

void handle_dlcount(int client_fd, const char *args) {
    char songname[256] = {0};

    if (sscanf(args, "%255s", songname) != 1) {
        dprintf(client_fd, "ERR Usage: DLCOUNT <song>\n");
        return;
    }

    if (!song_exists(songname)) {
    dprintf(client_fd, "ERR Song '%s' does not exist.\n", songname);
    return;
      }

    int count = get_download_count(songname);
    if (count >= 0) {
        dprintf(client_fd, "Download count for '%s': %d\n", songname, count);
    } else {
        dprintf(client_fd, "ERR Could not retrieve download count.\n");
    }
}

int song_exists(const char *songname) {
    for (int i = 0; i < song_count; ++i) {
        if (strcmp(song_index[i].filename, songname) == 0) {
            return 1;
        }
    }
    return 0;
}

int remove_song_from_index(const char *filename) {
    for (int i = 0; i < song_count; ++i) {
        if (strcmp(song_index[i].filename, filename) == 0) {
            // Shift songs left
            for (int j = i; j < song_count - 1; ++j) {
                song_index[j] = song_index[j + 1];
            }
            song_count--;
            return 1;
        }
    }
    return 0;
}

int rename_song_in_indexes(const char *old_filename, const char *new_filename) {
    for (int i = 0; i < song_count; i++) {
        if (strcmp(song_index[i].filename, old_filename) == 0) {
            // Found the song, update filename
            strncpy(song_index[i].filename, new_filename, sizeof(song_index[i].filename) - 1);
            song_index[i].filename[sizeof(song_index[i].filename) - 1] = '\0'; // null-terminate
            return 0; // success
        }
    }

    // Not found
    return -1;
}
