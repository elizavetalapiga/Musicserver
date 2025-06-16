#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

void handle_rcv(int sock_fd, const char *command);
void handle_rcv_list(int sock_fd);
void handle_rcv_get(int sock_fd, const char *filename);
void handle_snd_add(int sock_fd, const char *filename);
void handle_rcv_delete(int sock_fd);
void handle_rcv_rename(int sock_fd);
void handle_rcv_tag(int sock_fd);
const char* get_genre_name(unsigned char genre);
void handle_search_response(int sock_fd);
void handle_response(int response);


#endif
