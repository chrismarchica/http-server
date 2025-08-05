#include <stdio.h>
#include <stdlib.h>
#include "utils/server.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include "utils/parse_req.h"
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>
#include "utils/client.h"
#include "utils/http.h"

#ifdef USE_SSL
#include "utils/ssl.h"
SSL_CTX *global_ssl_ctx = NULL;  // Global SSL context
#endif

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
void handle_client_request(int client_fd, void *ssl)
{
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    
    ssize_t bytes_read;
    
#ifdef USE_SSL
    // If SSL context is available, try to detect if this is an HTTPS request
    if (ssl == NULL && global_ssl_ctx != NULL) {
        // Peek at the first few bytes to detect SSL handshake
        char peek_buffer[10];
        ssize_t peek_bytes = recv(client_fd, peek_buffer, sizeof(peek_buffer), MSG_PEEK);
        
        if (peek_bytes > 0) {
            // Check if this looks like an SSL handshake (starts with 0x16 for SSL/TLS)
            if (peek_buffer[0] == 0x16) {
                printf("Detected SSL handshake, attempting SSL connection...\n");
                ssl = create_ssl_connection(global_ssl_ctx, client_fd);
                if (ssl) {
                    printf("HTTPS connection established\n");
                } else {
                    printf("SSL connection failed, falling back to HTTP\n");
                }
            } else {
                printf("Detected HTTP request\n");
            }
        }
    }
    
    if (ssl) {
        bytes_read = ssl_read((SSL*)ssl, buffer, sizeof(buffer)-1);
    } else {
        bytes_read = read(client_fd, buffer, sizeof(buffer)-1);
    }
#else
    bytes_read = read(client_fd, buffer, sizeof(buffer)-1);
#endif
    
    if(bytes_read < 0)
    {
        perror("read error");
#ifdef USE_SSL
        if (ssl) {
            close_ssl_connection((SSL*)ssl);
        } else {
            close(client_fd);
        }
#else
        close(client_fd);
#endif
        return;
    }
    
    // Parse full HTTP request
    http_request_t req;
    if(!parse_full_request(buffer, &req)){
        printf("Could not parse request\n");
#ifdef USE_SSL
        if (ssl) {
            send_error_response_ssl(ssl, HTTP_STATUS_400, "Invalid request format");
        } else {
            send_error_response(client_fd, HTTP_STATUS_400, "Invalid request format");
        }
        if (ssl) {
            close_ssl_connection(ssl);
        } else {
            close(client_fd);
        }
#else
        send_error_response(client_fd, HTTP_STATUS_400, "Invalid request format");
        close(client_fd);
#endif
        return;
    }
    
    printf("Method: %s\n", req.method);
    printf("Path: %s\n", req.path);
    if (req.query_string[0] != '\0') {
        printf("Query: %s\n", req.query_string);
    }
    
    // Handle OPTIONS requests for CORS
    if (strcmp(req.method, "OPTIONS") == 0) {
#ifdef USE_SSL
        if (ssl) {
            // TODO: Implement SSL CORS headers
            close_ssl_connection(ssl);
        } else {
            send_cors_headers(client_fd);
            close(client_fd);
        }
#else
        send_cors_headers(client_fd);
        close(client_fd);
#endif
        return;
    }
    
    // Check if method is supported
    if (!is_supported_method(req.method)) {
#ifdef USE_SSL
        if (ssl) {
            send_error_response_ssl(ssl, HTTP_STATUS_405, "Method not allowed");
        } else {
            send_error_response(client_fd, HTTP_STATUS_405, "Method not allowed");
        }
        if (ssl) {
            close_ssl_connection(ssl);
        } else {
            close(client_fd);
        }
#else
        send_error_response(client_fd, HTTP_STATUS_405, "Method not allowed");
        close(client_fd);
#endif
        return;
    }
    
    // Route to API endpoints first
    int result = -1;
    
    // Health check endpoint
    if (strcmp(req.path, "/health") == 0) {
#ifdef USE_SSL
        if (ssl) {
            result = handle_api_health(client_fd, ssl);
        } else {
            result = handle_api_health(client_fd, NULL);
        }
#else
        result = handle_api_health(client_fd, NULL);
#endif
    }
    // Metrics endpoint
    else if (strcmp(req.path, "/metrics") == 0) {
#ifdef USE_SSL
        if (ssl) {
            result = handle_api_metrics(client_fd, ssl);
        } else {
            result = handle_api_metrics(client_fd, NULL);
        }
#else
        result = handle_api_metrics(client_fd, NULL);
#endif
    }
    // Users API
    else if (strncmp(req.path, "/api/users", 10) == 0) {
#ifdef USE_SSL
        if (ssl) {
            result = handle_api_users(client_fd, ssl, req.method, req.path, &req);
        } else {
            result = handle_api_users(client_fd, NULL, req.method, req.path, &req);
        }
#else
        result = handle_api_users(client_fd, NULL, req.method, req.path, &req);
#endif
    }
    // Fall back to static file serving for GET requests
    else if (strcmp(req.method, HTTP_METHOD_GET) == 0) {
#ifdef USE_SSL
        if (ssl) {
            result = handle_get_request(client_fd, ssl, req.path);
        } else {
            result = handle_get_request(client_fd, NULL, req.path);
        }
#else
        result = handle_get_request(client_fd, NULL, req.path);
#endif
    } else if (strcmp(req.method, HTTP_METHOD_POST) == 0) {
#ifdef USE_SSL
        if (ssl) {
            result = handle_post_request(client_fd, ssl, req.path);
        } else {
            result = handle_post_request(client_fd, NULL, req.path);
        }
#else
        result = handle_post_request(client_fd, NULL, req.path);
#endif
    } else if (strcmp(req.method, HTTP_METHOD_PUT) == 0) {
#ifdef USE_SSL
        if (ssl) {
            result = handle_put_request(client_fd, ssl, req.path);
        } else {
            result = handle_put_request(client_fd, NULL, req.path);
        }
#else
        result = handle_put_request(client_fd, NULL, req.path);
#endif
    } else if (strcmp(req.method, HTTP_METHOD_DELETE) == 0) {
#ifdef USE_SSL
        if (ssl) {
            result = handle_delete_request(client_fd, ssl, req.path);
        } else {
            result = handle_delete_request(client_fd, NULL, req.path);
        }
#else
        result = handle_delete_request(client_fd, NULL, req.path);
#endif
    }
    
    if (result != 0) {
        printf("Error handling request: %s %s\n", req.method, req.path);
#ifdef USE_SSL
        if (ssl) {
            send_error_response_ssl(ssl, HTTP_STATUS_404, "Not found");
        } else {
            send_error_response(client_fd, HTTP_STATUS_404, "Not found");
        }
#else
        send_error_response(client_fd, HTTP_STATUS_404, "Not found");
#endif
    }
    
#ifdef USE_SSL
    if (ssl) {
        close_ssl_connection(ssl);
    } else {
        close(client_fd);
    }
#else
    close(client_fd);
#endif
}        

// Thread pool implementation
thread_pool_t *create_thread_pool(int thread_count, int queue_size, void *ssl_ctx) {
    thread_pool_t *pool = malloc(sizeof(thread_pool_t));
    if (!pool) {
        perror("Failed to allocate thread pool");
        return NULL;
    }
    
    pool->thread_count = thread_count;
    pool->queue_size = queue_size;
    pool->queue_front = 0;
    pool->queue_rear = 0;
    pool->queue_count = 0;
    pool->shutdown = 0;
#ifdef USE_SSL
    pool->ssl_ctx = (SSL_CTX*)ssl_ctx;
#else
    (void)ssl_ctx; // Suppress unused parameter warning
#endif
    
    // Allocate thread array
    pool->threads = malloc(thread_count * sizeof(pthread_t));
    if (!pool->threads) {
        perror("Failed to allocate threads");
        free(pool);
        return NULL;
    }
    
    // Allocate client queue
    pool->client_queue = malloc(queue_size * sizeof(int));
    if (!pool->client_queue) {
        perror("Failed to allocate client queue");
        free(pool->threads);
        free(pool);
        return NULL;
    }
    
#ifdef USE_SSL
    // Allocate SSL queue
    pool->ssl_queue = malloc(queue_size * sizeof(SSL*));
    if (!pool->ssl_queue) {
        perror("Failed to allocate SSL queue");
        free(pool->client_queue);
        free(pool->threads);
        free(pool);
        return NULL;
    }
#endif
    
    // Initialize mutex and condition variable
    if (pthread_mutex_init(&pool->queue_mutex, NULL) != 0) {
        perror("Failed to initialize mutex");
        free(pool->client_queue);
        free(pool->threads);
        free(pool);
        return NULL;
    }
    
    if (pthread_cond_init(&pool->queue_cond, NULL) != 0) {
        perror("Failed to initialize condition variable");
        pthread_mutex_destroy(&pool->queue_mutex);
        free(pool->client_queue);
        free(pool->threads);
        free(pool);
        return NULL;
    }
    
    // Create worker threads
    for (int i = 0; i < thread_count; i++) {
        if (pthread_create(&pool->threads[i], NULL, worker_thread, pool) != 0) {
            perror("Failed to create worker thread");
            destroy_thread_pool(pool);
            return NULL;
        }
    }
    
    printf("Thread pool created with %d threads\n", thread_count);
    return pool;
}

void destroy_thread_pool(thread_pool_t *pool) {
    if (!pool) return;
    
    // Signal shutdown
    pthread_mutex_lock(&pool->queue_mutex);
    pool->shutdown = 1;
    pthread_cond_broadcast(&pool->queue_cond);
    pthread_mutex_unlock(&pool->queue_mutex);
    
    // Wait for all threads to finish
    for (int i = 0; i < pool->thread_count; i++) {
        pthread_join(pool->threads[i], NULL);
    }
    
    // Cleanup
    pthread_mutex_destroy(&pool->queue_mutex);
    pthread_cond_destroy(&pool->queue_cond);
    free(pool->client_queue);
#ifdef USE_SSL
    free(pool->ssl_queue);
#endif
    free(pool->threads);
    free(pool);
    printf("Thread pool destroyed\n");
}

int add_client_to_pool(thread_pool_t *pool, int client_fd, void *ssl) {
    if (!pool) return -1;
    
    pthread_mutex_lock(&pool->queue_mutex);
    
    // Check if queue is full
    if (pool->queue_count >= pool->queue_size) {
        pthread_mutex_unlock(&pool->queue_mutex);
        printf("Thread pool queue is full, rejecting client\n");
        return -1;
    }
    
    // Add client to queue
    pool->client_queue[pool->queue_rear] = client_fd;
#ifdef USE_SSL
    pool->ssl_queue[pool->queue_rear] = (SSL*)ssl;
#else
    (void)ssl; // Suppress unused parameter warning
#endif
    pool->queue_rear = (pool->queue_rear + 1) % pool->queue_size;
    pool->queue_count++;
    
    // Signal worker thread
    pthread_cond_signal(&pool->queue_cond);
    pthread_mutex_unlock(&pool->queue_mutex);
    
    return 0;
}

void *worker_thread(void *arg) {
    thread_pool_t *pool = (thread_pool_t *)arg;
    
    while (1) {
        int client_fd = -1;
        void *ssl = NULL;
        
        pthread_mutex_lock(&pool->queue_mutex);
        
        // Wait for client or shutdown signal
        while (pool->queue_count == 0 && !pool->shutdown) {
            pthread_cond_wait(&pool->queue_cond, &pool->queue_mutex);
        }
        
        // Check for shutdown
        if (pool->shutdown) {
            pthread_mutex_unlock(&pool->queue_mutex);
            pthread_exit(NULL);
        }
        
        // Get client from queue
        client_fd = pool->client_queue[pool->queue_front];
#ifdef USE_SSL
        ssl = pool->ssl_queue[pool->queue_front];
#endif
        pool->queue_front = (pool->queue_front + 1) % pool->queue_size;
        pool->queue_count--;
        
        pthread_mutex_unlock(&pool->queue_mutex);
        
        // Handle client request
        printf("Thread %lu handling client %d\n", pthread_self(), client_fd);
        handle_client_request(client_fd, ssl);
    }
    
    return NULL;
}        
