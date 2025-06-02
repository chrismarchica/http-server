#include <stdio.h>
#include <stdlib.h>
#include "server.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "parse_req.h"
#include <unistd.h>


int create_client(int server_fd)
{
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);

    if(client_fd < 0)
    {
        perror("client failure");
        return -1;
    }
    return client_fd;
}
void handle_client_request(int client_fd)
{
  char buffer[4096];
  memset(buffer, 0, sizeof(buffer));
  ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer)-1);
  if(bytes_read < 0)
  {
    perror("read error");
    close(client_fd);
    return;
  }
  char method[16];
  char path[256];
  if(parse_request(buffer, method, sizeof(method), path, sizeof(path))){
    printf("Method: %s\n", method);
    printf("Path: %s\n", path);
  }
  else
  {
    printf("Could not parse\n");
  }
    //Response
    const char *body = "Hello World!\n";
    char response[1024];
    snprintf(response, sizeof(response),
	     "HTTP/1.1 200 OK\r\n"
	     "Content-Type: text/plain\r\n"
	     "Content-Length: %lu\r\n"
	     "\r\n"
	     "%s",
	     strlen(body),body);
    write(client_fd, response, strlen(response));
    close(client_fd);
}        
