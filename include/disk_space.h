#ifndef DISK_SPACE_H
#define DISK_SPACE_H

int check_disk_space(const char *path, long required_bytes, long *disk_space);

#endif