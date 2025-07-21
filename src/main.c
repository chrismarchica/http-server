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
#include "utils/ssl.h"

// Forward declaration
void init_metrics(void);

// Global thread pool for signal handler
thread_pool_t *global_pool = NULL;
ssl_config_t ssl_config = {0};

// Signal handler for graceful shutdown
void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down gracefully...\n", sig);
    if (global_pool) {
        destroy_thread_pool(global_pool);
    }
#ifdef USE_SSL
    if (ssl_config.ssl_enabled) {
        cleanup_ssl(&ssl_config);
    }
#endif
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
   
   // Initialize SSL if certificate files exist
   ssl_config.cert_file = "server.crt";
   ssl_config.key_file = "server.key";
   
#ifdef USE_SSL
   if (generate_self_signed_cert(ssl_config.cert_file, ssl_config.key_file) == 0) {
       if (init_ssl(&ssl_config) == 0) {
           printf("HTTPS enabled on port 3000\n");
           // Set global SSL context for client detection
           extern SSL_CTX *global_ssl_ctx;
           global_ssl_ctx = ssl_config.ctx;
       } else {
           printf("HTTPS disabled - SSL initialization failed\n");
           ssl_config.ssl_enabled = 0;
       }
   } else {
       printf("HTTPS disabled - certificate generation failed\n");
       ssl_config.ssl_enabled = 0;
   }
#else
   printf("HTTPS disabled - OpenSSL not available\n");
   ssl_config.ssl_enabled = 0;
#endif
   
   // Create thread pool (4 threads, queue size of 20)
#ifdef USE_SSL
   global_pool = create_thread_pool(4, 20, ssl_config.ssl_enabled ? ssl_config.ctx : NULL);
#else
   global_pool = create_thread_pool(4, 20, NULL);
#endif
   if (!global_pool) {
       perror("Failed to create thread pool");
       close(server_fd);
       return 1;
   }

   // Initialize server metrics
   init_metrics();

   printf("Multi-threaded HTTP server ready to accept connections...\n");
   printf("Available endpoints:\n");
   printf("  GET  /                    - Serve static files\n");
   printf("  GET  /health              - Health check\n");
   printf("  GET  /metrics             - Server metrics\n");
   printf("  GET  /api/users           - List all users\n");
   printf("  GET  /api/users/{id}      - Get specific user\n");
   printf("  POST /api/users           - Create new user\n");
   printf("  PUT  /api/users/{id}      - Update user\n");
   printf("  DELETE /api/users/{id}    - Delete user\n");

   while(1)
   {
       //create client socket and accept
       int client_fd = create_client(server_fd);

       if (client_fd < 0)
       {
          perror("accept");
          continue;
       }
       
       // Don't create SSL connection here - let the client handler detect HTTP vs HTTPS
       void *ssl = NULL;
       
       // Add client to thread pool instead of handling directly
       if (add_client_to_pool(global_pool, client_fd, ssl) != 0) {
           printf("Failed to add client to thread pool, closing connection\n");
#ifdef USE_SSL
           if (ssl) {
               close_ssl_connection((SSL*)ssl);
           } else {
               close(client_fd);
           }
#else
           close(client_fd);
#endif
       }
   }

   // Cleanup (this won't be reached in normal operation due to signal handler)
   if (global_pool) {
       destroy_thread_pool(global_pool);
   }
   close(server_fd);
   
   return 0;
}   
