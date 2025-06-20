#include "network_utils.h"

// Creates a socket and returns its file descriptor
int create_socket() {
  int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd== -1){
    handle_error("Socket creation failed");
  }
  return sock_fd;
}

//Handles errors by printing a message and exiting the program
void handle_error(const char *message) {
  perror(message);
  exit(EXIT_FAILURE);
}

//Sets Up Server Address
void configure_server(struct sockaddr_in *server_addr, int port, const char *ip_address){
  server_addr->sin_family = AF_INET;
  server_addr->sin_port = htons(port);

  if (ip_address == NULL) {
    server_addr->sin_addr.s_addr = INADDR_ANY; // Accept connections from any IP
  } else {
    if (inet_pton(AF_INET, ip_address, &server_addr->sin_addr) <= 0){
      handle_error("Invalid server IP address");
    }
  }
}

//Sets Up client Address
void configure_client(struct sockaddr_in *server_addr, int port, const char *server_ip){
  server_addr->sin_family = AF_INET;
  server_addr->sin_port = htons(port);

  if (inet_pton(AF_INET, server_ip, &server_addr->sin_addr) <= 0){    
    handle_error("Invalid server IP address\n");
  }
}

int load_config(char *ip_buffer, size_t ip_buf_size, int *port_out) {
    FILE *file = fopen("config.txt", "r");
    if (!file) {
        perror("Failed to open config file");
        return 0;
    }

    char line[128];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "IP=", 3) == 0) {
            strncpy(ip_buffer, line + 3, ip_buf_size - 1);
            ip_buffer[strcspn(ip_buffer, "\n")] = '\0';  // trim newline
            ip_buffer[strcspn(ip_buffer, "\r\n")] = '\0'; // trims both \n and \r
        } else if (strncmp(line, "PORT=", 5) == 0) {
            *port_out = atoi(line + 5);
        }
    }

    fclose(file);
    return 1;
}
