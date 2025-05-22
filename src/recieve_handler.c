#include "recieve_handler.h"
#include "response_codes.h"
#include "network_utils.h"
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/socket.h>

void handle_rcv(int sock_fd, const char *command) {
    if (strcasecmp(command, "LIST") == 0) {
        handle_rcv_list(sock_fd);
    }
    else if (strncasecmp(command, "GET ", 4) == 0) {
        handle_rcv_get(sock_fd, command + 4); // filename follows "GET "
    }
    else if (strncasecmp(command, "DELETE ", 7) == 0) {
        handle_rcv_delete(sock_fd); 
    }
    else if (strncasecmp(command, "RENAME ", 7) == 0) {
      handle_rcv_rename(sock_fd);
    }
    else if (strncasecmp(command, "CREATEUSER ", 11) == 0) {
        handle_rcv_delete(sock_fd); 
    }
    
}

void handle_rcv_list(int sock_fd) {
  char buffer[512] = {0};
  size_t bytes_received;
  char *end_marker;
  int respond = 0;

  if (recv(sock_fd, &respond, sizeof(respond), 0) <= 0) {
    perror("Failed to receive server response");
    return;
    }  
  if (respond != OK) {
    handle_response(respond);  // handle the specific error
    return;  // Exit without entering the loop
  }   

  while ((bytes_received = recv(sock_fd, buffer, sizeof(buffer) - 1, 0)) > 0){
    buffer[bytes_received] = '\0'; // Null-terminate to use printf
    end_marker = strstr(buffer, "END\n");
    if (end_marker != NULL) {
      *end_marker = '\0';
      printf("%s", buffer);
      break;
    }
    printf("%s", buffer);
     }

    if (bytes_received <= 0) {
        perror("Error while receiving file list");
    }
  }


void handle_rcv_get(int sock_fd, const char *filename){
char buffer[1024] = {0};
long bytes_received;
char path[256] = {0};
long filesize;
long received_total = 0;


  //reciving filesize or error
  recv(sock_fd, &filesize, sizeof(filesize), 0);
  if (filesize == -1) {
  handle_response(ERR_FILE_NOT_FOUND);
  return;
  }
  printf("[DEBUG] Sending filesize: %ld\n", filesize);
  //building the path
  snprintf(path, sizeof(path), "client_music/%s", filename);

  // opening directory
  FILE *fp = fopen(path, "wb");
  if (!fp) {
        perror("File open failed");
        return;
    }

    //recieving file
    while (received_total < filesize) {
      bytes_received = recv(sock_fd, buffer, sizeof(buffer), 0);
      if (bytes_received <= 0){
        perror("Recieve failed");
        break;
      }

      fwrite(buffer, 1, bytes_received, fp);
      received_total += bytes_received;
    }

    fclose(fp);
    printf("File received: %ld bytes\n", received_total);
}

void handle_snd_add(int sock_fd, const char *filename){
  char path[512];
  int response = 0; 
  FILE *file;
  long filesize;
  char buffsndr[1024];
  size_t reads_bytes;
 
   
  snprintf(path, sizeof(path), "client_music/%s", filename);
 

  if ((file = fopen(path, "rb")) == NULL) {
       handle_error("File opening failed");
  }
 
    //filesize collection ans sending in order to stop the loop on client side
  fseek(file, 0, SEEK_END);
  filesize = ftell(file);
  rewind(file); //reset pointer to the begining of file
  send(sock_fd, &filesize, sizeof(filesize), 0);


  // Sending chunks + Error handeling
  while ((reads_bytes = fread(buffsndr, 1, sizeof(buffsndr), file)) > 0){
    if ((send(sock_fd, buffsndr, reads_bytes, 0)) == -1)
      perror("Add failed");
  }
  printf("[DEBUG] Received response: %d\n", response);


  if (recv(sock_fd, &response, sizeof(response), 0) > 0){
   handle_response(response);
  } 
    fclose(file);   
  }

  void handle_rcv_delete(int sock_fd) {
    int response = 0;
    if (recv(sock_fd, &response, sizeof(response), 0) > 0) {
      handle_response(response);
    } else {
        perror("Failed to receive response");
    }
}


void handle_rcv_rename(int sock_fd) {
    int response = 0;
    if (recv(sock_fd, &response, sizeof(response), 0) > 0) {
        handle_response(response);
    } else {
        perror("Failed to receive response");
    }
}

void handle_rcv_newuser(int sock_fd) {
    int response = 0;
    if (recv(sock_fd, &response, sizeof(response), 0) > 0) {
        handle_response(response);
    } else {
        perror("Failed to receive response");
    }
}

void handle_response(int response) {
  switch (response) {
    case OK:
    printf("Success\n");
    break;
    case ERR_GENERIC:
    printf("Error occured\n");
    break;
    case ERR_PERMISSION:
    printf("Error: Permission denied\n");
    break;
    case ERR_FILE_NOT_FOUND:
    printf("Error: File not found\n");
    break;
    case ERR_INVALID_ROLE:
    printf("Error: Invalid role\n");
    break;
    case ERR_PARSE:
    printf("Error: Command was not parsed\n");
    break;
    case ERR_USER_EXSISTS:
    printf("Error: User is already exists\n");
    break;
    case ERR_FILE_OPEN_FAIL:
    printf("Error: File open failed\n");
    break;
    case ERR_INCOMPLETE_TRANSFER:
    printf("Error: Transfer of file failed\n");
    break;
  }
}