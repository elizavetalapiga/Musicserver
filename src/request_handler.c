#include "request_handler.h"
#include "network_utils.h"
#include "login.h"
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/socket.h>
#include <unistd.h>


void handle_cmd(int client_fd, const char *command, int *logged_in, const char *role){
      // Always allow LOGIN
      if (strncasecmp(command, "LOGIN ", 6) == 0) {
        *logged_in = handle_login(client_fd, command, role); // returns 1 if successful
        return;
    }

    // Always allow LOGOUT
    if (strcasecmp(command, "LOGOUT") == 0) {
        *logged_in = 0;
        send(client_fd, "You have been logged out.\n", 27, 0);
        return;
    }

    // Block all other commands if not logged in
    if (!*logged_in) {
        send(client_fd, "ERROR: Please LOGIN first.\n", 28, 0);
        return;
    }

  //other commands
   if (strcasecmp(command, "LIST") == 0) {
        handle_list(client_fd);
    }
    else if (strncasecmp(command, "GET ", 4) == 0) {
        handle_get(client_fd, command + 4);
    }
    else if (strncasecmp(command, "ADD ", 4) == 0) {
      handle_add(client_fd, command, role);
      return;
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

int handle_login(int client_fd, const char *command, *role_out) {
  char username[64], password[64];
  char role[64] = "";
  char response[64];

  // Try to parse: LOGIN <username> <password>
  if (sscanf(command + 6, "%63s %63s", username, password) != 2) {
      send(client_fd, "ERROR: Invalid login format\n", 28, 0);
      return 0;
  }
  printf("[DEBUG] LOGIN: user='%s', pass='%s'\n", username, password);


  if (check_credentials(username, password, role)) {
      strcpy(out_role, role); // passing role back to caller
      
      snprintf(response, sizeof(response), "LOGIN OK [%s]\n", role);
      send(client_fd, response, strlen(response), 0);

      printf("[DEBUG] Role = '%s'\n", role);
      return 1;
  } else {
      send(client_fd, "LOGIN FAIL\n", 11, 0);
      return 0;
  }
}

void handle_add(int client_fd, const char *command, const char *role) {
  if (strcmp(role, "admin") != 0) {
      send(client_fd, "ERROR: Admins only\n", 19, 0);
      return;
  }

  // TODO: implement file receive and write
}
