#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

void handle_rcv(int sock_fd, const char *command);
void handle_rcv_list(int sock_fd);
void handle_rcv_get(int sock_fd, const char *filename);
void handle_snd_add(int sock_fd, const char *filename);
void handle_rcv_delete(int sock_fd);
void handle_rcv_rename(int sock_fd);
void handle_rcv_tag(int sock_fd);
void handle_search_response(int sock_fd);
void handle_response(int response);
void handle_rcv_changetag(int sock_fd);
void recv_and_print(int sock_fd);
void handle_rcv_rate(int sock_fd);
void handle_rcv_avg(int sock_fd);
void handle_rcv_dlcount(int sock_fd);
void recv_and_print(int sock_fd);

#endif
