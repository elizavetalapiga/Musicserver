#include "network_utils.h"
#include "recieve_handler.h"
#include "login_client.h"

#define PORT 8080
#define IP_SERVER "127.0.0.1"


int main() {
  int sock_fd = 0;
  struct  sockaddr_in server_addr;
  char command[512];


  // Create a socket + error handeling
  sock_fd = create_socket();


  configure_client(&server_addr, PORT, IP_SERVER);


  // Connect to server
  if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
    handle_error("Connection failed");
    }

  printf("Connected to the server\n");

  // login
  if (!client_login(sock_fd)) {
    printf("Exiting due to failed login.\n");
    close(sock_fd);
    return 1;
  }

  // command sending
  while (1) {
    // clear the array
    memset(command, 0, sizeof(command));

    printf("Enter the command: list, get <song_name>, logout, login <user> <pass>\n");
    fgets(command, sizeof(command), stdin);

    // Remove newline character
    command[strcspn(command,"\n")]= '\0';

    // Sends messeges
    if ((send(sock_fd , command, strlen(command), 0)) == -1){
      handle_error("Send failed");
      break;
    }
    printf("[DEBUG] Sent command: %s\n", command);
    handle_rcv(sock_fd, command);

  }

  close(sock_fd);
  return 0;
}
