#include <stdio.h>
#include <stdlib.h>
#include "server.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>




int main()
{
   setvbuf(stdout, NULL, _IONBF, 0);
   int server_fd = init_server(3000);

   while(1)
   {
       struct sockaddr_in client_addr;
       socklen_t client_len = sizeof(client_addr);
       
       int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
       
       if (client_fd < 0)
       {
	  perror("accept");
          continue;
       }
       
   }
   close(server_fd);
   
}   
