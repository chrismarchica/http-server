#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int init_server(int port)
{  //declare struct and socket
   int server_file_desc;
   struct sockaddr_in address;
   //Stream socket, IPv4, and TCP
   server_file_desc = socket(AF_INET, SOCK_STREAM, 0);
   //Catch error if socket fails to be made
   if(server_file_desc < 0)
   {
       printf("error\n");
       exit(EXIT_FAILURE);
   }
   //clear memory
   memset(&address, 0, sizeof(address));
   //configure struct
   address.sin_family = AF_INET;
   address.sin_addr.s_addr = INADDR_ANY;
   address.sin_port = htons(port);
   //bind socket to address and port

   int bind_ = bind(server_file_desc, (struct sockaddr *)&address, sizeof(address));
   //check for failure
   if(bind_ < 0)
   {	   
      perror("bind");
      close(server_file_desc);
      exit(EXIT_FAILURE);
   }
   //now listen
   int listen_ = listen(server_file_desc, 10);
   if(listen_ < 0)
   {
       perror("listen");
       close(server_file_desc);
       exit(EXIT_FAILURE);
   }
   //return the socket
   return server_file_desc;

}   
