#ifndef CLIENT_H
#define CLIENT_H

#include <pthread.h>

#ifdef USE_SSL
#include <openssl/ssl.h>
extern SSL_CTX *global_ssl_ctx;  // Global SSL context
#endif

// Thread pool structure
typedef struct {
    pthread_t *threads;
    int thread_count;
    int *client_queue;
#ifdef USE_SSL
    SSL **ssl_queue;  // SSL connections for each client
#endif
    int queue_size;
    int queue_front;
    int queue_rear;
    int queue_count;
    pthread_mutex_t queue_mutex;
    pthread_cond_t queue_cond;
    int shutdown;
#ifdef USE_SSL
    SSL_CTX *ssl_ctx;  // SSL context for the thread pool
#endif
} thread_pool_t;

// Function declarations
int create_client(int server_fd);
void handle_client_request(int client_fd, void *ssl);
void *worker_thread(void *arg);
thread_pool_t *create_thread_pool(int thread_count, int queue_size, void *ssl_ctx);
void destroy_thread_pool(thread_pool_t *pool);
int add_client_to_pool(thread_pool_t *pool, int client_fd, void *ssl);

#endif
