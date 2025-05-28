#include <stdio.h>
#include <stdlib.h>
#include "server.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "parse_req.h"
#include <unistd.h>
#include "client.h"

int main()
{
   setvbuf(stdout, NULL, _IONBF, 0);
   int server_fd = init_server(3000);

   while(1)
   {
       //create client socket and accept
       int client_fd = create_client(server_fd);
       //create buffer and clear it
       char buffer[4096];
       memset(buffer, 0, sizeof(buffer));

       if (client_fd < 0)
       {
	  perror("accept");
          continue;
       }
       ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer)-1);
       if(bytes_read < 0)
       {
	  perror("read error");
	  close(client_fd);
	  continue;
       }
       //printf("raw http request:\n%s\n",buffer);

       char method[16];
       char path[256];
       if(parse_request(buffer, method, sizeof(method), path, sizeof(path)))
       {
	  printf("Method: %s\n", method);
	  printf("Path: %s\n",path);
       }
       else
       {
	  printf("Could not parse\n");
       }
   }

   close(server_fd);
   
}   
