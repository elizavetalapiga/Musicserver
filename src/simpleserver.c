
#include <signal.h>
#include "network_utils.h"
#include "request_handler.h"

#define PORT 8080
#define BUFFER_SIZE 1024

int main(){
  int sock_fd = 0, lstn_res = 0, bind_res = 0, client_fd = 0;
  socklen_t addr_len = 0;
  struct  sockaddr_in server_addr;
  struct  sockaddr_in client_addr;
  pid_t pid;
  char command[512];



  // Server config
  configure_server(&server_addr, 8080, NULL);  // Bind to all interfaces

  // Socket creation + Error handeling
  sock_fd = create_socket();

  // Bind the socket to an IP and PORT
  bind_res = bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));

  // Error handeling
  if (bind_res == -1) {
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

          //recieving filename
          if (recv(client_fd, command, sizeof(command), 0) <= 0) {
            close(client_fd);
            handle_error("Recieving failed");/* code */
          }
          handle_cmd(client_fd, command);
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
