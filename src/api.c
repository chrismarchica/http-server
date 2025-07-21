#include "utils/http.h"
#include "utils/parse_req.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#ifdef USE_SSL
#include "utils/ssl.h"
#endif

// Forward declaration
void init_metrics(void);

// Simple in-memory database for users
typedef struct {
    int id;
    char name[64];
    char email[128];
    char created_at[32];
} user_t;

#define MAX_USERS 100
static user_t users[MAX_USERS];
static int user_count = 0;
static pthread_mutex_t users_mutex = PTHREAD_MUTEX_INITIALIZER;

// Server metrics
typedef struct {
    int total_requests;
    int successful_requests;
    int error_requests;
    time_t start_time;
    pthread_mutex_t mutex;
} server_metrics_t;

static server_metrics_t metrics = {0, 0, 0, 0, PTHREAD_MUTEX_INITIALIZER};

// Initialize metrics
void init_metrics() {
    metrics.start_time = time(NULL);
}

// Update metrics
void update_metrics(int success) {
    pthread_mutex_lock(&metrics.mutex);
    metrics.total_requests++;
    if (success) {
        metrics.successful_requests++;
    } else {
        metrics.error_requests++;
    }
    pthread_mutex_unlock(&metrics.mutex);
}

// JSON helper functions
void send_json_response(int client_fd, void *ssl, const char *status, const char *json_data) {
    char response[8192];
    snprintf(response, sizeof(response),
             "%s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %zu\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
             "Access-Control-Allow-Headers: Content-Type\r\n"
             "\r\n"
             "%s",
             status, CONTENT_TYPE_JSON, strlen(json_data), json_data);
    
#ifdef USE_SSL
    if (ssl) {
        ssl_write((SSL*)ssl, response, strlen(response));
    } else {
        write(client_fd, response, strlen(response));
    }
#else
    write(client_fd, response, strlen(response));
#endif
}

void send_json_response_plain(int client_fd, const char *status, const char *json_data) {
    char response[8192];
    snprintf(response, sizeof(response),
             "%s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %zu\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
             "Access-Control-Allow-Headers: Content-Type\r\n"
             "\r\n"
             "%s",
             status, CONTENT_TYPE_JSON, strlen(json_data), json_data);
    
    write(client_fd, response, strlen(response));
}

void send_cors_headers(int client_fd) {
    const char *cors_headers = 
        "HTTP/1.1 200 OK\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Content-Length: 0\r\n"
        "\r\n";
    
    write(client_fd, cors_headers, strlen(cors_headers));
}

// Health check endpoint
int handle_api_health(int client_fd, void *ssl) {
    time_t uptime = time(NULL) - metrics.start_time;
    
    char json_response[512];
    snprintf(json_response, sizeof(json_response),
             "{\"status\": \"healthy\", \"uptime\": %ld, \"timestamp\": \"%s\"}\n",
             uptime, ctime(&metrics.start_time));
    
#ifdef USE_SSL
    if (ssl) {
        send_json_response(client_fd, ssl, HTTP_STATUS_200, json_response);
    } else {
        send_json_response_plain(client_fd, HTTP_STATUS_200, json_response);
    }
#else
    send_json_response_plain(client_fd, HTTP_STATUS_200, json_response);
#endif
    update_metrics(1);
    return 0;
}

// Metrics endpoint
int handle_api_metrics(int client_fd, void *ssl) {
    pthread_mutex_lock(&metrics.mutex);
    int total = metrics.total_requests;
    int success = metrics.successful_requests;
    int errors = metrics.error_requests;
    time_t uptime = time(NULL) - metrics.start_time;
    pthread_mutex_unlock(&metrics.mutex);
    
    char json_response[1024];
    snprintf(json_response, sizeof(json_response),
             "{\"total_requests\": %d, \"successful_requests\": %d, "
             "\"error_requests\": %d, \"uptime_seconds\": %ld, "
             "\"success_rate\": %.2f}\n",
             total, success, errors, uptime,
             total > 0 ? (float)success / total * 100 : 0.0);
    
#ifdef USE_SSL
    if (ssl) {
        send_json_response(client_fd, ssl, HTTP_STATUS_200, json_response);
    } else {
        send_json_response_plain(client_fd, HTTP_STATUS_200, json_response);
    }
#else
    send_json_response_plain(client_fd, HTTP_STATUS_200, json_response);
#endif
    update_metrics(1);
    return 0;
}

// User management functions
int create_user(const char *name, const char *email) {
    pthread_mutex_lock(&users_mutex);
    
    if (user_count >= MAX_USERS) {
        pthread_mutex_unlock(&users_mutex);
        return -1;
    }
    
    user_t *user = &users[user_count];
    user->id = user_count + 1;
    strncpy(user->name, name, sizeof(user->name) - 1);
    strncpy(user->email, email, sizeof(user->email) - 1);
    
    time_t now = time(NULL);
    strftime(user->created_at, sizeof(user->created_at), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    user_count++;
    pthread_mutex_unlock(&users_mutex);
    return user->id;
}

user_t* get_user(int id) {
    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < user_count; i++) {
        if (users[i].id == id) {
            pthread_mutex_unlock(&users_mutex);
            return &users[i];
        }
    }
    pthread_mutex_unlock(&users_mutex);
    return NULL;
}

int update_user(int id, const char *name, const char *email) {
    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < user_count; i++) {
        if (users[i].id == id) {
            if (name) strncpy(users[i].name, name, sizeof(users[i].name) - 1);
            if (email) strncpy(users[i].email, email, sizeof(users[i].email) - 1);
            pthread_mutex_unlock(&users_mutex);
            return 0;
        }
    }
    pthread_mutex_unlock(&users_mutex);
    return -1;
}

int delete_user(int id) {
    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < user_count; i++) {
        if (users[i].id == id) {
            // Shift remaining users
            for (int j = i; j < user_count - 1; j++) {
                users[j] = users[j + 1];
            }
            user_count--;
            pthread_mutex_unlock(&users_mutex);
            return 0;
        }
    }
    pthread_mutex_unlock(&users_mutex);
    return -1;
}

// Convert user to JSON
void user_to_json(const user_t *user, char *json, size_t size) {
    snprintf(json, size,
             "{\"id\": %d, \"name\": \"%s\", \"email\": \"%s\", \"created_at\": \"%s\"}\n",
             user->id, user->name, user->email, user->created_at);
}

// Convert all users to JSON array
void users_to_json_array(char *json, size_t size) {
    char *ptr = json;
    int written = snprintf(ptr, size, "[");
    ptr += written;
    size -= written;
    
    pthread_mutex_lock(&users_mutex);
    for (int i = 0; i < user_count && size > 0; i++) {
        if (i > 0) {
            written = snprintf(ptr, size, ",");
            ptr += written;
            size -= written;
        }
        
        written = snprintf(ptr, size,
                          "{\"id\": %d, \"name\": \"%s\", \"email\": \"%s\", \"created_at\": \"%s\"}",
                          users[i].id, users[i].name, users[i].email, users[i].created_at);
        ptr += written;
        size -= written;
    }
    pthread_mutex_unlock(&users_mutex);
    
    snprintf(ptr, size, "]\n");
}

// Parse JSON-like body for user creation/update
int parse_user_json(const char *json, char *name, size_t name_size, char *email, size_t email_size) {
    // Initialize with empty strings
    name[0] = '\0';
    email[0] = '\0';
    
    // Find "name" field
    const char *name_field = strstr(json, "\"name\":");
    if (name_field) {
        // Skip past "name": and any whitespace
        name_field += 7; // length of "\"name\":"
        while (*name_field == ' ' || *name_field == '\t') name_field++;
        
        // Check if it's a quoted string
        if (*name_field == '"') {
            name_field++; // skip opening quote
            const char *name_end = strchr(name_field, '"');
            if (name_end) {
                size_t name_len = name_end - name_field;
                if (name_len < name_size) {
                    strncpy(name, name_field, name_len);
                    name[name_len] = '\0';
                }
            }
        }
    }
    
    // Find "email" field
    const char *email_field = strstr(json, "\"email\":");
    if (email_field) {
        // Skip past "email": and any whitespace
        email_field += 8; // length of "\"email\":"
        while (*email_field == ' ' || *email_field == '\t') email_field++;
        
        // Check if it's a quoted string
        if (*email_field == '"') {
            email_field++; // skip opening quote
            const char *email_end = strchr(email_field, '"');
            if (email_end) {
                size_t email_len = email_end - email_field;
                if (email_len < email_size) {
                    strncpy(email, email_field, email_len);
                    email[email_len] = '\0';
                }
            }
        }
    }
    
    // Return true if both name and email were successfully parsed
    return (name[0] != '\0' && email[0] != '\0');
}

// Users API endpoint
int handle_api_users(int client_fd, void *ssl, const char *method, const char *path, const http_request_t *req) {
    if (strcmp(method, "GET") == 0) {
        if (strcmp(path, "/api/users") == 0) {
            // Get all users
            char json_response[4096];
            users_to_json_array(json_response, sizeof(json_response));
#ifdef USE_SSL
            if (ssl) {
                send_json_response(client_fd, ssl, HTTP_STATUS_200, json_response);
            } else {
                send_json_response_plain(client_fd, HTTP_STATUS_200, json_response);
            }
#else
            send_json_response_plain(client_fd, HTTP_STATUS_200, json_response);
#endif
            update_metrics(1);
            return 0;
        } else if (strncmp(path, "/api/users/", 11) == 0) {
            // Get specific user
            int user_id = atoi(path + 11);
            user_t *user = get_user(user_id);
            
            if (user) {
                char json_response[512];
                user_to_json(user, json_response, sizeof(json_response));
#ifdef USE_SSL
                if (ssl) {
                    send_json_response(client_fd, ssl, HTTP_STATUS_200, json_response);
                } else {
                    send_json_response_plain(client_fd, HTTP_STATUS_200, json_response);
                }
#else
                send_json_response_plain(client_fd, HTTP_STATUS_200, json_response);
#endif
                update_metrics(1);
            } else {
#ifdef USE_SSL
                if (ssl) {
                    send_json_response(client_fd, ssl, HTTP_STATUS_404, "{\"error\": \"User not found\"}\n");
                } else {
                    send_json_response_plain(client_fd, HTTP_STATUS_404, "{\"error\": \"User not found\"}\n");
                }
#else
                send_json_response_plain(client_fd, HTTP_STATUS_404, "{\"error\": \"User not found\"}\n");
#endif
                update_metrics(0);
            }
            return 0;
        }
    } else if (strcmp(method, "POST") == 0 && strcmp(path, "/api/users") == 0) {
        // Create new user with JSON body parsing
        char name[64] = "John Doe";
        char email[128] = "john@example.com";
        
        // Try to parse JSON from request body if available
        if (req->body[0] != '\0') {
            if (parse_user_json(req->body, name, sizeof(name), email, sizeof(email))) {
                printf("Creating user: %s (%s)\n", name, email);
            } else {
                printf("Failed to parse JSON, using default values\n");
            }
        }
        
        int user_id = create_user(name, email);
        if (user_id > 0) {
            char json_response[512];
            snprintf(json_response, sizeof(json_response),
                     "{\"message\": \"User created successfully\", \"id\": %d, \"name\": \"%s\", \"email\": \"%s\"}\n", 
                     user_id, name, email);
#ifdef USE_SSL
            if (ssl) {
                send_json_response(client_fd, ssl, HTTP_STATUS_201, json_response);
            } else {
                send_json_response_plain(client_fd, HTTP_STATUS_201, json_response);
            }
#else
            send_json_response_plain(client_fd, HTTP_STATUS_201, json_response);
#endif
            update_metrics(1);
        } else {
#ifdef USE_SSL
            if (ssl) {
                send_json_response(client_fd, ssl, HTTP_STATUS_500, "{\"error\": \"Failed to create user\"}\n");
            } else {
                send_json_response_plain(client_fd, HTTP_STATUS_500, "{\"error\": \"Failed to create user\"}\n");
            }
#else
            send_json_response_plain(client_fd, HTTP_STATUS_500, "{\"error\": \"Failed to create user\"}\n");
#endif
            update_metrics(0);
        }
        return 0;
    } else if (strcmp(method, "PUT") == 0 && strncmp(path, "/api/users/", 11) == 0) {
        // Update user
        int user_id = atoi(path + 11);
        if (update_user(user_id, "Updated Name", "updated@example.com") == 0) {
#ifdef USE_SSL
            if (ssl) {
                send_json_response(client_fd, ssl, HTTP_STATUS_200, "{\"message\": \"User updated successfully\"}\n");
            } else {
                send_json_response_plain(client_fd, HTTP_STATUS_200, "{\"message\": \"User updated successfully\"}\n");
            }
#else
            send_json_response_plain(client_fd, HTTP_STATUS_200, "{\"message\": \"User updated successfully\"}\n");
#endif
            update_metrics(1);
        }
        else {
#ifdef USE_SSL
            if (ssl) {
                send_json_response(client_fd, ssl, HTTP_STATUS_404, "{\"error\": \"User not found\"}\n");
            } else {
                send_json_response_plain(client_fd, HTTP_STATUS_404, "{\"error\": \"User not found\"}\n");
            }
#else
            send_json_response_plain(client_fd, HTTP_STATUS_404, "{\"error\": \"User not found\"}\n");
#endif
            update_metrics(0);
        }
        return 0;
    } else if (strcmp(method, "DELETE") == 0 && strncmp(path, "/api/users/", 11) == 0) {
        // Delete user
        int user_id = atoi(path + 11);
        if (delete_user(user_id) == 0) {
#ifdef USE_SSL
            if (ssl) {
                send_json_response(client_fd, ssl, HTTP_STATUS_204, "\n");
            } else {
                send_json_response_plain(client_fd, HTTP_STATUS_204, "\n");
            }
#else
            send_json_response_plain(client_fd, HTTP_STATUS_204, "\n");
#endif
            update_metrics(1);
        } else {
#ifdef USE_SSL
            if (ssl) {
                send_json_response(client_fd, ssl, HTTP_STATUS_404, "{\"error\": \"User not found\"}\n");
            } else {
                send_json_response_plain(client_fd, HTTP_STATUS_404, "{\"error\": \"User not found\"}\n");
            }
#else
            send_json_response_plain(client_fd, HTTP_STATUS_404, "{\"error\": \"User not found\"}\n");
#endif
            update_metrics(0);
        }
        return 0;
    }
    
    return -1; // Not handled
} 