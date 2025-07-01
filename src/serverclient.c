#include "network_utils.h"
#include "recieve_handler.h"
#include "login_client.h"
#include "cache_handler.h"



int main() {
  int sock_fd = 0;
  struct  sockaddr_in server_addr;
  char command[1024], filename[256];
  char ip[64];
  int port;

  if (!load_config(ip, sizeof(ip), &port)) {
      exit(EXIT_FAILURE);
  }

  
  // Create a socket 
  sock_fd = create_socket();
  configure_client(&server_addr, port, ip);

  // Connect to server
  if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1){
    printf("Connection failed");
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

    printf("Enter the command: list - list all songs on the server\n"
      "play <song_name> - download and play a song from the server\n"
      "logout - logout from the account\n"
      "login <user> <pass> - log into account\n"
      "info <song_name> - tag info query\n"
      "search <album/artist/year/genre> <value> - search for songs by tag\n"
      "rate <song_name> <1-5> - rate the song\n"
      "avg <song_name> - rating statisctics\n"
      "dlcount <song_name> - downloading statistics\n"
      "For admin:\n"
      "add <song_name> - upload the song to the server\n"
      "delete <song_name> - delete the song from the server\n"
      "rename <song_name new_name> - rename the song on the server\n"
      "createuser <name> <password> <role> - create a new user account\n"
      "changetag <song_name> <album/artist/year/genre> <value> - change the tag of the song\n");
    fgets(command, sizeof(command), stdin);

    // Remove newline character (fgets includes it because client press Enter)
    command[strcspn(command,"\n")]= '\0';
    
    if ((strncasecmp(command, "PLAY ", 5) == 0) && (check_cache(command + 5) == 1)) {
      printf("[DEBUG] Playing from cache: %s\n", command + 5);
      handle_play(command + 5); // filename follows "PLAY "
      continue; // Skip sending the command to the server
    }

    if ((strncasecmp(command, "EXIT", 4) == 0)  || (strncasecmp(command, "LOGOUT", 6) == 0)) {
      printf("[DEBUG] Cleaning cache\n");
      cleanup_cache();
      if (strncasecmp(command, "EXIT", 4) == 0) {
        printf("Exiting the client.\n");
        break; // Exit the loop and close the socket
      } 
    }

    // Sends commands
    if ((send(sock_fd , command, strlen(command), 0)) == -1){
      printf("Send failed");
      break;
    }

    //add function call
    if (strncasecmp(command, "ADD ", 4) == 0) {      
      sscanf(command + 4, "%255s", filename);
      handle_snd_add(sock_fd, filename);
      continue;  
    }

    printf("[DEBUG] Sent command: %s\n", command);
    handle_rcv(sock_fd, command);

  }

  close(sock_fd);
  return 0;
}
