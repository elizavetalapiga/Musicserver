#include "cache_handler.h"
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>


int check_cache(const char *filename){
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "client_music/%s", filename);
    

    if (access(filepath, F_OK) == 0) {// Check if file exists in cache        
        return 1; // File exists in cache
    }
    
    return 0; // File does not exist in cache
}

void cleanup_cache(){
    DIR *dir = opendir("client_music");

    struct dirent *entry;// Directory entry structure, for selecting only regular files
    char filepath[512];

   while ((entry = readdir(dir)) != NULL) {
        // Only delete regular files
        if (entry->d_type == DT_REG) {
            snprintf(filepath, sizeof(filepath), "client_music/%s", entry->d_name);// full path with filename taken from entry
            if (remove(filepath) != 0) 
                printf("Failed to delete file: %s\n", filepath);            
        }
    }
    closedir(dir);
    printf("Cache clean up is finished\n");

}