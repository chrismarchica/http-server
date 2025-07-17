#include <stdio.h>
#include <stdlib.h>
#include "utils/server.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "utils/parse_req.h"
#include <unistd.h>
#include "utils/client.h"
#include <signal.h>

// Global thread pool for signal handler
thread_pool_t *global_pool = NULL;

// Signal handler for graceful shutdown
void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down gracefully...\n", sig);
    if (global_pool) {
        destroy_thread_pool(global_pool);
    }
    exit(0);
}

int main()
{
   setvbuf(stdout, NULL, _IONBF, 0);
   
   // Set up signal handler for graceful shutdown
   signal(SIGINT, signal_handler);
   signal(SIGTERM, signal_handler);
   
   int server_fd = init_server(3000);
   printf("Server started on port 3000\n");
   
   // Create thread pool (4 threads, queue size of 20)
   global_pool = create_thread_pool(4, 20);
   if (!global_pool) {
       perror("Failed to create thread pool");
       close(server_fd);
       return 1;
   }

   printf("Multi-threaded HTTP server ready to accept connections...\n");

   while(1)
   {
       //create client socket and accept
       int client_fd = create_client(server_fd);

       if (client_fd < 0)
       {
          perror("accept");
          continue;
       }
       
       // Add client to thread pool instead of handling directly
       if (add_client_to_pool(global_pool, client_fd) != 0) {
           printf("Failed to add client to thread pool, closing connection\n");
           close(client_fd);
       }
   }

   // Cleanup (this won't be reached in normal operation due to signal handler)
   if (global_pool) {
       destroy_thread_pool(global_pool);
   }
   close(server_fd);
   
   return 0;
}   
