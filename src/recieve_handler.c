#include "recieve_handler.h"
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/socket.h>

void handle_rcv(int sock_fd, const char *command) {
    if (strcasecmp(command, "LIST") == 0) {
        handle_rcv_list(sock_fd);
    }
    else if (strncasecmp(command, "get ", 4) == 0) {
        handle_rcv_get(sock_fd, command + 4); // filename follows "GET "
    }
}

void handle_rcv_list(int sock_fd) {
  char buffer[512] = {0};
  size_t bytes_received;
  char *end_marker;
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
  printf("Server: File not found\n");
  return;
  }
  printf("[DEBUG] Sending filesize: %ld\n", filesize);
  //building the path
  snprintf(path, sizeof(path), "downloads/%s", filename);

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
