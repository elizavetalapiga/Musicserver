#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

void handle_cmd(int client_fd, const char *command);
void handle_list(int client_fd);
void handle_get(int client_fd, const char *filename);

#endif
