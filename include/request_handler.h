#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

void handle_cmd(int client_fd, const char *command, int *logged_in, char *role, char *username);
void handle_list(int client_fd);
void handle_get(int client_fd, const char *filename);
void handle_add(int client_fd, const char *command, const char *role);
void handle_delete(int client_fd, const char *command, const char *role);
void handle_rename(int client_fd, const char *command, const char *role);
void handle_newuser(int client_fd, const char *command, const char *role);
int handle_login(int client_fd, const char *command, char *role_out, char *username_out);
void handle_info(int client_fd, const char *filename);
void handle_rate(int client_fd, const char *args, const char *user);
void handle_avg(int client_fd, const char *args);
void handle_dlcount(int client_fd, const char *args);
int song_exists(const char *songname);
int remove_song_from_index(const char *filename);
#endif
