#include "disk_space.h"
#include <sys/statvfs.h>

int check_disk_space(const char *path, long required_bytes, long *disk_space){
    struct statvfs fs; //file system statistics structure

    if (statvfs(path, &fs) != 0) {
        return 0;// return error if it fails
    }

    *disk_space = fs.f_bsize * fs.f_bavail; //Filesystem block size * Number of free blocks 
    return *disk_space >= required_bytes; // return 1 (true) if its true or 0 (false) if its false
}