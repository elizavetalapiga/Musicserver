
#include <signal.h>
#include "network_utils.h"
#include "request_handler.h"

#define PORT 8080


int main(){
  int sock_fd = 0, lstn_res = 0, client_fd = 0;
  socklen_t addr_len = 0;
  struct  sockaddr_in server_addr;
  struct  sockaddr_in client_addr;
  pid_t pid;
  char command[512];
  int logged_in = 0;
  int opt = 1;
  char role[64] = "";  // store role for this client



  // Server config
  configure_server(&server_addr, 8080, NULL);  // Bind to all interfaces

  // Socket creation + Error handeling
  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd == -1) {
    handle_error("Socket creation failed");
  }
 
  //Fre port for reuse
  setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  // Bind the socket to an IP and PORT
  if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
    handle_error("Bind failed");
  }

    // Listen for incoming connections
  lstn_res = listen(sock_fd, 3);

      // Error handeling
  if (lstn_res == -1) {
    close(sock_fd);
    handle_error("listen failed");
    }

  printf("Server is listening on port %d \n", PORT);

  // SIGCHLD - Signal that a child process has terminated or changed state; SIG_IGN - Ignore this signal. Against zombi processes
  signal(SIGCHLD, SIG_IGN);

  // (1) - in this case run forver
    while (1) {
      addr_len = sizeof(client_addr);

      // Accept traffic + Error handeling
      if ((client_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &addr_len)) == -1) {
        handle_error("Accept failed");
        continue; // try again
      }

      pid = fork();
      if (pid == 0) { //child process
        close(sock_fd);
        
        while (1) {
          // clear the array
          memset(command, 0, sizeof(command));

        int bytes = recv(client_fd, command, sizeof(command), 0);
        
        if (bytes == 0) {
            printf("Client disconnected.\n");
            break;
        }
        if (bytes < 0) {
            perror("recv failed");
            break;
        }
        command[bytes] = '\0';  // Null-terminate it
        printf("[DEBUG] Received command: '%s'\n", command);
         handle_cmd(client_fd, command, &logged_in, role);
        }

        close(client_fd);
        exit(0);
        } else {
          if (pid > 0)
            close(client_fd);
          else
            handle_error("Fork failed");
        }
      }


  return 0;
  }
