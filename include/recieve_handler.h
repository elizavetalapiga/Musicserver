#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

void handle_rcv(int sock_fd, const char *command);
void handle_rcv_list(int sock_fd);
void handle_rcv_get(int sock_fd, const char *filename);

#endif
