#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

void handle_cmd(int client_fd, const char *command, int *logged_in, const char *role);
void handle_list(int client_fd);
void handle_get(int client_fd, const char *filename);
void handle_add(int client_fd, const char *command, const char *role);
void handle_delete(int client_fd, const char *command, const char *role);
void handle_rename(int client_fd, const char *command, const char *role);
void handle_newuser(int client_fd, const char *command, const char *role);
#endif
