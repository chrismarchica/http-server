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
    if(!parse_request(buffer, method, sizeof(method), path, sizeof(path))){
        printf("Could not parse request\n");
        send_error_response(client_fd, HTTP_STATUS_400, "Invalid request format");
        close(client_fd);
        return;
    }
    
    printf("Method: %s\n", method);
    printf("Path: %s\n", path);
    
    // Check if method is supported
    if (!is_supported_method(method)) {
        send_error_response(client_fd, HTTP_STATUS_405, "Method not allowed");
        close(client_fd);
        return;
    }
    
    // Route to appropriate handler based on HTTP method
    int result = -1;
    if (strcmp(method, HTTP_METHOD_GET) == 0) {
        result = handle_get_request(client_fd, path);
    } else if (strcmp(method, HTTP_METHOD_POST) == 0) {
        result = handle_post_request(client_fd, path);
    } else if (strcmp(method, HTTP_METHOD_PUT) == 0) {
        result = handle_put_request(client_fd, path);
    } else if (strcmp(method, HTTP_METHOD_DELETE) == 0) {
        result = handle_delete_request(client_fd, path);
    }
    
    if (result != 0) {
        printf("Error handling request: %s %s\n", method, path);
    }
    
    close(client_fd);
}        

// Thread pool implementation
thread_pool_t *create_thread_pool(int thread_count, int queue_size) {
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
    free(pool->threads);
    free(pool);
    printf("Thread pool destroyed\n");
}

int add_client_to_pool(thread_pool_t *pool, int client_fd) {
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
        pool->queue_front = (pool->queue_front + 1) % pool->queue_size;
        pool->queue_count--;
        
        pthread_mutex_unlock(&pool->queue_mutex);
        
        // Handle client request
        printf("Thread %lu handling client %d\n", pthread_self(), client_fd);
        handle_client_request(client_fd);
    }
    
    return NULL;
}        
