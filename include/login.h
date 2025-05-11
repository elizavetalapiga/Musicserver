#ifndef LOGIN
#define LOGIN

int check_credentials(const char *username, const char *password, char *role_out);
int handle_login(int client_fd, const char *command, char *role_out);


#endif
