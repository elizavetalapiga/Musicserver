#ifndef TAG_HANDLER_H
#define TAG_HANDLER_H

#include <stdint.h>


struct ID3v1Tag {
    char tag[3];      // "TAG"
    char title[31];
    char artist[31];
    char album[31];
    char year[5];
    unsigned char genre;
};

void read_id3v1_tag(int client_fd, const char *command);
void send_tag_info(int client_fd, const struct ID3v1Tag *tag);

#endif 