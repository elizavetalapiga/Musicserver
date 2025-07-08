#include "recieve_handler.h"
#include "response_codes.h"
#include "network_utils.h"
#include "tag_handler.h"
#include "cache_handler.h"
#include "disk_space.h"
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void handle_rcv(int sock_fd, const char *command) {
    if (strcasecmp(command, "LIST") == 0) {
        handle_rcv_list(sock_fd);
    }
    else if (strncasecmp(command, "PLAY ", 5) == 0) {
        handle_rcv_get(sock_fd, command + 5); // filename follows "GET "
    }
    else if (strncasecmp(command, "DELETE ", 7) == 0) {
        handle_rcv_delete(sock_fd,command + 7); 
    }
    else if (strncasecmp(command, "RENAME ", 7) == 0) {
      handle_rcv_rename(sock_fd);
    }
    else if (strncasecmp(command, "CREATEUSER ", 11) == 0) {
        handle_rcv_newuser(sock_fd); 
    }
    else if (strncasecmp(command, "INFO ", 5) == 0) {
        handle_rcv_tag(sock_fd); 
    }
    else if (strncasecmp(command, "SEARCH ", 7) == 0) {
        handle_search_response(sock_fd); 
    }
    else if (strncasecmp(command, "CHANGETAG ", 10) == 0) {
        handle_rcv_changetag(sock_fd); // tag type and new value follows "CHANGETAG "
    }
    else if (strncasecmp(command, "RATE ", 5) == 0) {
    handle_rcv_rate(sock_fd);  // after "RATE "
    }
    else if (strncasecmp(command, "AVG ", 4) == 0) {
    handle_rcv_avg(sock_fd);  // after "AVG "
    }
    else if (strncasecmp(command, "DLCOUNT ", 8) == 0) {
    handle_rcv_dlcount(sock_fd);  // after "DLCOUNT "
    }
      else {
    int response = ERR_GENERIC;  // Default to generic error if recv fails;
    recv(sock_fd, &response, sizeof(response), 0);
    handle_response(response);        
    }
    
}

void handle_rcv_list(int sock_fd) {
  char buffer[512] = {0};
  char line[512] = {0};
  size_t line_len = 0;
  ssize_t bytes_received;
  int respond ;

  if (recv(sock_fd, &respond, sizeof(respond), 0) <= 0) {
    handle_response(ERR_RESPONSE_RECV_FAIL);
    exit(EXIT_FAILURE);
    }  
    
  if (respond != OK) {
    handle_response(respond);  // handle the specific error
    return;  // Exit without entering the loop
  }   

  while ((bytes_received = recv(sock_fd, buffer, sizeof(buffer), 0)) > 0){
    for (ssize_t i = 0; i < bytes_received; ++i) { 
            if (buffer[i] == '\n') {
                line[line_len] = '\0';
                if (strcmp(line, "END") == 0) return;
                printf("%s\n", line);  // each song
                line_len = 0;  // reset for next line
            } else if (line_len < sizeof(line) - 1) {
                line[line_len++] = buffer[i];
            }
        }
    }


    if (bytes_received <= 0) {
        perror("Error while receiving file list\n");
        return;
    }
  }


void handle_rcv_get(int sock_fd, const char *filename){
char buffer[1024] = {0};
long bytes_received;
char path[512] = {0};
long filesize;
long received_total = 0;
long disk_space = 0;
int response;


  //reciving response
  if (recv(sock_fd, &response, sizeof(response), 0)== -1) {
    handle_response(ERR_RESPONSE_RECV_FAIL);
    exit(EXIT_FAILURE);
  }
  if (response != OK) {
  handle_response(response);
  return;
  }

  //reciving filesize or error
  recv(sock_fd, &filesize, sizeof(filesize), 0);
  if (filesize == -1) {
  handle_response(ERR_GENERIC);
  return;
  }

  if (check_disk_space("client_music", filesize, &disk_space) == 0){
    printf("Not enough disk space to save the file, total available space: %ld bytes.\n", disk_space);
    printf("File '%ld' not saved.\n", filesize);
    response = ERR_DISK_IS_FULL;
    send(sock_fd, &response, sizeof(response), 0); 
    return;
  }

  //building the path
  snprintf(path, sizeof(path), "client_music/%s", filename);

  // opening directory
  FILE *fp = fopen(path, "wb");
  if (!fp) {
    response = ERR_FILE_OPEN_FAIL;
    send(sock_fd, &response, sizeof(response), 0); 
    return;
    }

  response = OK;
  send(sock_fd, &response, sizeof(response), 0);

  //recieving file
  while (received_total < filesize) {
    bytes_received = recv(sock_fd, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0){
    perror("Recieved failed");
      fclose(fp);
      remove(path);
      printf("File transfer incomplete, file '%s' not saved. Try again\n", filename);
      return; 
    }

    fwrite(buffer, 1, bytes_received, fp);
    received_total += bytes_received;
  }

  fclose(fp);
  printf("File received: %ld bytes\n", received_total);  
  handle_play(filename); //play the song after recieving it    
  }

void handle_play(const char *filename) {
    
    pid_t pid = fork();

    if (pid == 0) {
        // In child process
        const char *cmd = "ffplay";
        char filepath[256] = {0};
        snprintf(filepath, sizeof(filepath), "client_music/%s", filename);

        char *args[] = {
            "ffplay", "-nodisp", "-autoexit", filepath, NULL
        };

        execvp(cmd, args); // run the ffplay with passed arguments, in other words play the song

        // If exec fails
        perror("execvp failed");
        _exit(1);
    } else if (pid > 0) {
        // In parent: wait for child
        waitpid(pid, NULL, 0);
    } else {
        perror("fork failed");
    }
}

void handle_snd_add(int sock_fd, const char *filename){
  char path[512] = {0};
  int response = 0;  
  int state;
   
  snprintf(path, sizeof(path), "client_music/%s", filename);

  if (recv(sock_fd, &response, sizeof(response), 0) <= 0) {
    handle_response(ERR_RESPONSE_RECV_FAIL);
    exit(EXIT_FAILURE); 
    }
  
  if (response != OK) {
      handle_response(response);  
      return;  
    } 

  state = send_file(sock_fd, path);
  if (state <= 0) {
      printf("Resending the file\n");
      state = send_file(sock_fd, path);
      if (state != OK)
          printf("Resending failed\n");
  }
 
  return;  
}

  int send_file(int sock_fd, const char *filename) {
    size_t reads_bytes;
    int response = 0;
    long filesize;
    char buffsndr[1024] = {0};

    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
       perror("File opening failed");
       return -1;
  } 
  //filesize collection ans sending in order to stop the loop on client side
  fseek(file, 0, SEEK_END);
  filesize = ftell(file);
  rewind(file); //reset pointer to the begining of file
  send(sock_fd, &filesize, sizeof(filesize), 0);

  // Sending chunks + Error handeling
  while ((reads_bytes = fread(buffsndr, 1, sizeof(buffsndr), file)) > 0){
    if ((send(sock_fd, buffsndr, reads_bytes, 0)) == -1){
      perror("Add failed");
      return -1;  
    }   
  }
  fclose(file);

  if (recv(sock_fd, &response, sizeof(response), 0) <= 0) {
  perror("No response or connection closed");
  return -1; 
  }

  if (response == ERR_INCOMPLETE_TRANSFER) {
    handle_response(response);
    return 0;
  } else if (response == OK) {
    handle_response(response);
    return 1;
  }
  return -1; 
}


  void handle_rcv_delete(int sock_fd, const char *filename) {
    int response = 0;
    char filepath[512] = {0};

    if (recv(sock_fd, &response, sizeof(response), 0) <= 0) {
    handle_response(ERR_RESPONSE_RECV_FAIL);
    exit(EXIT_FAILURE);
      }
    if (response != OK) {
      handle_response(response);
      return;
    }
      
  //crafting the filepath
  snprintf(filepath, sizeof(filepath), "client_music/%s", filename);

  //removing file, sending the respond
  if (remove(filepath) == 0) {
    printf("%s was deleted on server and client side\n", filename);
  } else {
    printf("[INFO] '%s' deleted on server, but not found in local cache.\n", filename);
    }
}


void handle_rcv_rename(int sock_fd) {
    int response = 0;
    if (recv(sock_fd, &response, sizeof(response), 0) > 0) {
        handle_response(response);
    } else {
        handle_response(ERR_RESPONSE_RECV_FAIL);
        exit(EXIT_FAILURE);
    }
}

void handle_rcv_newuser(int sock_fd) {
    int response = 0;
    if (recv(sock_fd, &response, sizeof(response), 0) > 0) {
        handle_response(response);
    } else {
        handle_response(ERR_RESPONSE_RECV_FAIL);
        exit(EXIT_FAILURE);
    }
}

void handle_rcv_tag(int sock_fd) {
    int response = 0;
    char buffer [512] = {0};
    
    // Receive response code
    if (recv(sock_fd, &response, sizeof(response), 0) <= 0) {
        handle_response(ERR_RESPONSE_RECV_FAIL);
        exit(EXIT_FAILURE);
    }

    if (response != OK) {
        handle_response(response);
        return; // Exit if there was an error
    }

    // Receive tag data
    if (recv(sock_fd, buffer, sizeof(buffer), 0) <= 0) {
        handle_response(ERR_TAG_PARSE_FAIL);
        return;
    }

    // Print the received tag info
    printf("%s\n", buffer);
}


void handle_search_response(int sock_fd){
  char buffer[1024] = {0};
  size_t bytes_received;
  int respond = 0;

  if (recv(sock_fd, &respond, sizeof(respond), 0) <= 0) {
    handle_response(ERR_RESPONSE_RECV_FAIL);
    exit(EXIT_FAILURE);
  }  
  if (respond != OK) {
    handle_response(respond);  // handle the specific error
    return;  // Exit without entering the loop
  }   

  while ((bytes_received = recv(sock_fd, buffer, sizeof(buffer) - 1, 0)) > 0) {
      buffer[bytes_received] = '\0';

      char *line = strtok(buffer, "\n"); // split the buffer into lines
      while (line != NULL) {
          if (strcmp(line, "END") == 0) return; // done
          printf("%s\n", line);
          line = strtok(NULL, "\n");
      }
  }  
    if (bytes_received <= 0) {
        handle_response(ERR_RESPONSE_RECV_FAIL);
    }
}


void handle_rcv_changetag(int sock_fd){
  int response = 0;
  struct ID3v1Tag tag;
  // Receive response code
  if (recv(sock_fd, &response, sizeof(response), 0) <= 0) {
    handle_response(ERR_RESPONSE_RECV_FAIL);
    exit(EXIT_FAILURE);
}

if (response != OK) {
    handle_response(response);
    return;
}


   // Receive tag data
  if (recv(sock_fd, &tag, sizeof(tag), 0) <= 0) {
    handle_response(ERR_TAG_PARSE_FAIL);
    exit(EXIT_FAILURE);
  }

  // Print updated tag information
  printf("Updated ID3v1 Tag Information:\n");
  printf("Title: %s\n", tag.title);
  printf("Artist: %s\n", tag.artist);
  printf("Album: %s\n", tag.album);
  printf("Year: %s\n", tag.year);  
  printf("Genre: %d\n", tag.genre);    
  printf("Genre: %s\n", get_genre_name(tag.genre));
  fflush(stdout);
}

void recv_and_print(int sock_fd) {
    char buffer[256];
    int bytes = recv(sock_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        printf("%s", buffer);
    } else if (bytes == 0) {
        printf("Connection closed by server.\n");
    } else {
        handle_response(ERR_RESPONSE_RECV_FAIL);
        exit(EXIT_FAILURE);
    }
  }

void handle_rcv_rate(int sock_fd) {
    recv_and_print(sock_fd);
}
void handle_rcv_avg(int sock_fd) {
    recv_and_print(sock_fd);
}
void handle_rcv_dlcount(int sock_fd) {
    recv_and_print(sock_fd);
}


void handle_response(int response) {
  switch (response) {
    case OK:
    printf("Success\n");
    break;
    case ERR_GENERIC:
    printf("Error occured\n");
    break;
    case ERR_PERMISSION:
    printf("Error: Permission denied\n");
    break;
    case ERR_FILE_NOT_FOUND:
    printf("Error: File not found\n");
    break;
    case ERR_INVALID_ROLE:
    printf("Error: Invalid role\n");
    break;
    case ERR_PARSE:
    printf("Error: Command was not parsed\n");
    break;
    case ERR_USER_EXSISTS:
    printf("Error: User is already exists\n");
    break;
    case ERR_FILE_OPEN_FAIL:
    printf("Error: File open failed\n");
    break;
    case ERR_INCOMPLETE_TRANSFER:
    printf("Error: Transfer of file failed\n");
    break;
    case ERR_TAG_NOT_FOUND:
    printf("Error: ID3 tag not found\n");
    break;
    case ERR_TAG_PARSE_FAIL:
    printf("Error: Failed to parse ID3 tag\n");
    break;
    case ERR_RESPONSE_RECV_FAIL:
    printf("Error: Failed to receive response\n");
       break;
    case ERR_UNKNOWN_COMMAND:
    printf("Error: Unknown command\n");
      break;
    case ERR_FILE_EXISTS:
    printf("Error: File is already exist\n");
      break;
    case ERR_DISK_IS_FULL:
    printf("Error: Not enough space on a disk\n");
      break;
    case ERR_LOCK_FAILED:
    printf("Error: Failed to lock the file\n");
      break;
    case ERR_INVALID_TAG_TYPE:
    printf("Error: Invalid TAG parameter\n");
      break;  
  }


}
