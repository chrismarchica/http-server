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
    char *body = NULL;
    
    ssize_t body_len = 0;

    if(file){
	struct stat st;
	stat(full_path, &st);
	body_len = st.st_size;

	body = malloc(body_len);
	if(!body)
	{
	   perror("memory allocation failed");
	   close(client_fd);
	   fclose(file);
	   return;
	}	   


	fread(body, 1, body_len, file);
	fclose(file);

	status_line = "HTTP/1.1 200 OK";

	//define type
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
	 
        else if (strstr(full_path, ".png")) {
    	    content_type = "image/png";
	} 
	else if (strstr(full_path, ".jpg") || strstr(full_path, ".jpeg")) {
    	    content_type = "image/jpeg";
	} 
	else if (strstr(full_path, ".gif")) {
    	    content_type = "image/gif";
	}
	}
    else
    {
	//no file has been found
	
	const char *not_found = "404 not found";
	body_len = strlen(not_found);

	body = malloc(body_len);
	if(!body)
	{
	    perror("malloc failed");
	    close(client_fd);
	    return;
	}

	strncpy(body, "404 Not Found", body_len);
	
	status_line = "HTTP/1.1 404 Not Found";
	content_type = "text/plain";
    

    	memcpy(body, not_found, body_len);
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
    free(body);
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
