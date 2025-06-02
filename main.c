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
   int server_fd = init_server(8080);

   while(1)
   {
       //create client socket and accept
       int client_fd = create_client(server_fd);

       if (client_fd < 0)
       {
	  perror("accept");
          continue;
       }
       handle_client_request(client_fd);
   }

   close(server_fd);
   
}   
