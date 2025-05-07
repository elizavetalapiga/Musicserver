#include "request_handler.h"
#include "network_utils.h"
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/socket.h>
#include <unistd.h>


void handle_cmd(int client_fd, const char *command) {
    if (strcasecmp(command, "LIST") == 0) {
        handle_list(client_fd);
    }
    else if (strncasecmp(command, "GET ", 4) == 0) {
        handle_get(client_fd, command + 4); // filename follows "GET "
    }
}

void handle_list(int client_fd){
  DIR *dir = opendir("music/");
  char buffer[512];
  if (dir == NULL) {
    perror("opendir failed");
    return;// exit the function
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_REG)
    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "%s\n", entry->d_name);
    send(client_fd, buffer, strlen(buffer), 0);
  }
  send(client_fd, "END\n", 4, 0);

  closedir(dir);
}

void handle_get (int client_fd, const char *filename) {

  // Trim newline, since const was passed we cannot change filename directly
  char local_filename[256];
  strncpy(local_filename, filename, sizeof(local_filename));
  local_filename[sizeof(local_filename)-1] = '\0';  // safety null
  local_filename[strcspn(local_filename, "\n")] = '\0';

  //building the path
  char path[512];
  memset(path, 0, sizeof(path));
  snprintf(path, sizeof(path), "music/%s", local_filename);
  printf("Trying to open file: %s\n", path);

  //checking if the file can be open. RB - Read in Binary Mode, required for mp3
  FILE *file;
  if ((file = fopen(path, "rb")) == NULL) {
    //error reciving in case of non exsistance
    long filesize = -1;
    send(client_fd, &filesize, sizeof(filesize), 0);

    handle_error("File opening failed");
  }

  //filesize collection ans sending in order to stop the loop on client side
  fseek(file, 0, SEEK_END);
  long filesize = ftell(file);
  rewind(file); //reset pointer to the begining of file
  send(client_fd, &filesize, sizeof(filesize), 0);

  printf("[DEBUG] Sending filesize: %ld\n", filesize);

  char buffsndr[1024];
  size_t reads_bytes;
  // Sending chunks + Error handeling
  while ((reads_bytes = fread(buffsndr, 1, sizeof(buffsndr), file)) > 0){
    if ((send(client_fd, buffsndr, reads_bytes, 0)) == -1)
      perror("Send failed");

  }
  //check
  printf("File was sent: %s\n", path);
  fclose(file);
}
