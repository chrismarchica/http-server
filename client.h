#ifndef CLIENT_H
#define CLIENT_H
int create_client(int server_fd);
void handle_client_request(int client_fd);
#endif
