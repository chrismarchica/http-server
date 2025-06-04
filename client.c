#include <stdio.h>
#include <stdlib.h>
#include "server.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "parse_req.h"
#include <unistd.h>
#include <sys/stat.h>

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
    //Build file path
    char full_path[512];
    
    //write the path into full_path
    snprintf(full_path, sizeof(full_path), "static%s",path);
    //if given the root give index.html
    if(strcmp(path, "/") == 0){
	snprintf(full_path, sizeof(full_path), "static/index.html");
    }

    //open file to read the binary
    FILE *file = fopen(full_path, "rb");
    const char *status_line;
    const char *content_type = "text/plain"; //default

    char body[8192] = {0};
    ssize_t body_len = 0;

    if(file){
	struct stat st;
	stat(full_path, &st);
	body_len = st.st_size;

	fread(body, 1, body_len, file);
	fclose(file);

	status_line = "HTTP/1.1 200 OK";

	//define file type
	if(strstr(full_path, ".html")){
	    content_type = "text/html";	    
    	}
	else if(strstr(full_path, ".css"))
	{
	    content_type = "text/css";
	}
	else if (strstr(full_path, ".js"))
	{
	    content_type = "application/javascript";
	}
	}
    else
    {
	//no file has been found
	body_len = strlen(body);
	strncpy(body, "404 Not Found", body_len);
	
	status_line = "HTTP/1.1 404 Not Found";
	content_type = "text/plain";
    }
    //build response
    char response[4096] = {0};
    snprintf(response, sizeof(response),
	     "%s\r\n"
	     "Content-Type: %s\r\n"
	     "Content-Length: %lu\r\n"
	     "\r\n",
	     status_line, content_type, body_len);
    //send headers
    write(client_fd, response, strlen(response));
    //send body
    write(client_fd, body, body_len);
    close(client_fd);
}        
