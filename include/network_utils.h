#ifndef NETWORK_UTILS_H  // If NETWORK_UTILS_H is NOT defined
#define NETWORK_UTILS_H  // Define NETWORK_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// Function prototypes
int create_socket();
void handle_error(const char *message);
void configure_server(struct sockaddr_in *server_addr, int port, const char *ip_address);
void configure_client(struct sockaddr_in *server_addr, int port, const char *server_ip);
int load_config(char *ip_buffer, size_t ip_buf_size, int *port_out);

#endif
