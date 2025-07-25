#include "db_handler.h"
#include <signal.h>
#include "network_utils.h"
#include "request_handler.h"
#include "tag_handler.h"
#include "semaphore.h"
#include <sys/sem.h>


int main(){
  int sock_fd = 0, lstn_res = 0, client_fd = 0;
  socklen_t addr_len = 0;
  struct  sockaddr_in server_addr;
  struct  sockaddr_in client_addr;
  pid_t pid;
  char command[1024] = {0};
  int logged_in = 0;
  int opt = 1; // Allow socket to be reused
  char role[64] = {0};  // store role for this client
  char username[64] = {0};
  char ip[64] = {0};
  int port;

  memset(&server_addr, 0, sizeof(server_addr));
  memset(&client_addr, 0, sizeof(client_addr));



  if (!load_config(ip, sizeof(ip), &port)) {
      exit(EXIT_FAILURE);
  }

  // Server config
  configure_server(&server_addr, port, NULL);  // Bind to all interfaces

  // Socket creation + Error handeling
  sock_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_fd == -1) {
    handle_error("Socket creation failed");
  }
 
  //Free port for reuse
  setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  // Bind the socket to an IP and PORT
  if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
    handle_error("Bind failed");
  }

    // Listen for incoming connections
  lstn_res = listen(sock_fd, 3);

  
  if (lstn_res == -1) {
    close(sock_fd);
    handle_error("listen failed");
    }

  printf("Server is listening on port %d \n", port);

  
  signal(SIGCHLD, SIG_IGN);// SIGCHLD - Signal that a child process has terminated or changed state; SIG_IGN - Ignore this signal. Against zombi processes
  signal(SIGPIPE, SIG_IGN); // Ignore SIGPIPE to prevent crash on client disconnect


  init_song_index();  // Allocate the dynamic song index
  index_songs("music");  // This will use add_song_to_index()

  init_semaphore();



  // (1) - in this case run forver
    while (1) {
      addr_len = sizeof(client_addr);

      // Accept traffic + Error handeling
      if ((client_fd = accept(sock_fd, (struct sockaddr *)&client_addr, &addr_len)) == -1) {
        printf("Accept failed");
        continue; // try again
      }
      
      

      pid = fork();
      if (pid == 0) { //child process
        close(sock_fd);
        
        if (!init_database("music.db")) {
          fprintf(stderr, "Database initialization failed\n");
          exit(EXIT_FAILURE);
        }

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
            handle_cmd(client_fd, command, &logged_in, role, username);
          }

        close(client_fd);
        close_database();
        exit(0);
        } else {
          if (pid > 0)
            close(client_fd);
          else
            printf("Fork failed");
        }
      }

  free_song_index();  //cleanup song index     
  return 0;
  }
