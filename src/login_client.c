#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/socket.h>
#include <unistd.h>
#include "login_client.h"
#include "network_utils.h" //error handeling


int client_login(int sock_fd) {
  char username[64], password[64], login_cmd[150], response[128] = {0};
  int bytes;

  // Get user credentials
  printf("Enter username: ");
  scanf("%63s", username);
  printf("Enter password: ");
  scanf("%63s", password);
  getchar(); // clears leftover \n

  // Format and send LOGIN command
  snprintf(login_cmd, sizeof(login_cmd), "LOGIN %s %s", username, password);
  if (send(sock_fd, login_cmd, strlen(login_cmd), 0) == -1) {
      handle_error("Login send failed");
      return 0;
  }

  bytes = recv(sock_fd, response, sizeof(response) - 1, 0);
  if (bytes <= 0) {
    perror("Login response failed");
    return 0;
}
response[bytes] = '\0';  // null-terminate

  printf("[Server]: %s\n", response);

  if (strncmp(response, "LOGIN OK", 8) == 0)
      return 1;

  return 0;
}
