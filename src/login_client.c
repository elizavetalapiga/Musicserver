#include <string.h>
#include <stdio.h>
#include "response_codes.h"
#include <dirent.h>
#include <sys/socket.h>
#include <unistd.h>
#include "login_client.h"
#include "recieve_handler.h"
#include "network_utils.h" //error handeling


int client_login(int sock_fd) {
  char username[64], password[64], login_cmd[150];
  int response;

  // Get user credentials
  printf("Enter username: ");
  scanf("%63s", username);
  printf("Enter password: ");
  scanf("%63s", password);
  getchar(); // clears leftover \n

  // Format and send LOGIN command
  snprintf(login_cmd, sizeof(login_cmd), "LOGIN %s %s", username, password);
  if (send(sock_fd, login_cmd, strlen(login_cmd), 0) == -1) {
      printf("Login send failed");
      return 0;
  }

  
  if (recv(sock_fd, &response, sizeof(response), 0) <= 0) {
    perror("Login response failed");
    return 0;
  }
  printf("[DEBUG] Received login response code: %d\n", response);


  if (response == OK) {
    printf("[Server]: LOGIN OK\n");
    return 1; 
  }
  handle_response(response); 
  return 0;
 
}
