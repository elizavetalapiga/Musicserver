#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/socket.h>
#include <unistd.h>
#include "login.h"


int check_credentials(const char *username, const char *password, char *role_out){
  FILE *file = fopen("credentials.txt", "r");
   if (file == NULL) {
    perror("fopen failed");
    return 0; //exit the function
  } 
  char line [128];
  while (fgets(line, sizeof(line), file)){
    line[strcspn(line, "\n")] = '\0';  // Remove trailing newline from fgets

    char file_user[64] = {0}, file_pass[64] = {0}, file_role[64] = {0};
        if (sscanf(line, "%63[^:]:%63[^:]:%63s", file_user, file_pass, file_role) == 3) { 
          //sscanf parses line from the file. %63[^:] means read up to 63 characters and stop when it hits ":" 
          // and store it in file_user. Same for %63s. == 3 means the check that all value match the format and were extracted correctly
          
         
                      
          if (strcmp(file_user, username) == 0 && strcmp(file_pass, password) == 0) {
            strcpy(role_out, file_role); // copy role to caller    
            fclose(file);
                return 1;
            }
          } 

  }
  fclose(file);
  return 0;
  }


