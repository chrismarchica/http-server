#include "utils/http.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <limits.h>

void send_http_response(int client_fd, const char *status, const char *content_type, const char *body) {
    size_t body_len = body ? strlen(body) : 0;
    
    char response[4096] = {0};
    snprintf(response, sizeof(response),
             "%s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %zu\r\n"
             "\r\n",
             status, content_type, body_len);
    
    // Send headers
    write(client_fd, response, strlen(response));
    
    // Send body if present
    if (body && body_len > 0) {
        write(client_fd, body, body_len);
    }
}

void send_error_response(int client_fd, const char *status, const char *message) {
    char error_body[512];
    snprintf(error_body, sizeof(error_body), 
             "<html><body><h1>Error</h1><p>%s</p></body></html>", 
             message ? message : "An error occurred");
    
    send_http_response(client_fd, status, CONTENT_TYPE_HTML, error_body);
}

const char* get_content_type(const char *filename) {
    if (!filename) return CONTENT_TYPE_PLAIN;
    
    if (strstr(filename, ".html")) return CONTENT_TYPE_HTML;
    if (strstr(filename, ".css")) return CONTENT_TYPE_CSS;
    if (strstr(filename, ".js")) return CONTENT_TYPE_JS;
    if (strstr(filename, ".json")) return CONTENT_TYPE_JSON;
    if (strstr(filename, ".png")) return CONTENT_TYPE_PNG;
    if (strstr(filename, ".jpg") || strstr(filename, ".jpeg")) return CONTENT_TYPE_JPG;
    if (strstr(filename, ".gif")) return CONTENT_TYPE_GIF;
    
    return CONTENT_TYPE_PLAIN;
}

int is_supported_method(const char *method) {
    if (!method) return 0;
    
    return (strcmp(method, HTTP_METHOD_GET) == 0 ||
            strcmp(method, HTTP_METHOD_POST) == 0 ||
            strcmp(method, HTTP_METHOD_PUT) == 0 ||
            strcmp(method, HTTP_METHOD_DELETE) == 0);
}

// Handle GET request - serve static files
int handle_get_request(int client_fd, const char *path) {
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "src/static%s", path);
    
    // If root path, serve index.html
    if (strcmp(path, "/") == 0) {
        snprintf(full_path, sizeof(full_path), "src/static/index.html");
    }

    // Sanitize the file path to prevent directory traversal
    char resolved_path[PATH_MAX];
    if (realpath(full_path, resolved_path) == NULL) {
        send_error_response(client_fd, HTTP_STATUS_404, "File not found");
        return -1;
    }

    // Verify the resolved path is within the static directory
    char static_dir_abs[PATH_MAX];
    if (realpath("src/static", static_dir_abs) == NULL) {
        send_error_response(client_fd, HTTP_STATUS_500, "Internal server error");
        return -1;
    }

    if (strncmp(resolved_path, static_dir_abs, strlen(static_dir_abs)) != 0) {
        send_error_response(client_fd, HTTP_STATUS_403, "Forbidden");
        return -1;
    }
    
    FILE *file = fopen(resolved_path, "rb");
    if (!file) {
        send_error_response(client_fd, HTTP_STATUS_404, "File not found");
        return -1;
    }
    
    // Get file size
    struct stat st;
    if (stat(resolved_path, &st) != 0) {
        fclose(file);
        send_error_response(client_fd, HTTP_STATUS_500, "Failed to get file info");
        return -1;
    }
    
    size_t file_size = st.st_size;
    char *body = malloc(file_size);
    if (!body) {
        fclose(file);
        send_error_response(client_fd, HTTP_STATUS_500, "Memory allocation failed");
        return -1;
    }
    
    // Read file content
    size_t bytes_read = fread(body, 1, file_size, file);
    fclose(file);
    
    if (bytes_read != file_size) {
        free(body);
        send_error_response(client_fd, HTTP_STATUS_500, "Failed to read file");
        return -1;
    }
    
    // Send response
    const char *content_type = get_content_type(resolved_path);
    send_http_response(client_fd, HTTP_STATUS_200, content_type, body);
    free(body);
    
    return 0;
}

// Handle POST request - create new resource
int handle_post_request(int client_fd, const char *path) {
    // For now, just return a success response
    // In a real implementation, you'd parse the request body and create a resource
    const char *response_body = "{\"message\": \"Resource created successfully\", \"path\": \"";
    char full_response[1024];
    snprintf(full_response, sizeof(full_response), "%s%s\"}", response_body, path);
    
    send_http_response(client_fd, HTTP_STATUS_201, CONTENT_TYPE_JSON, full_response);
    return 0;
}

// Handle PUT request - update existing resource
int handle_put_request(int client_fd, const char *path) {
    // For now, just return a success response
    // In a real implementation, you'd parse the request body and update the resource
    const char *response_body = "{\"message\": \"Resource updated successfully\", \"path\": \"";
    char full_response[1024];
    snprintf(full_response, sizeof(full_response), "%s%s\"}", response_body, path);
    
    send_http_response(client_fd, HTTP_STATUS_200, CONTENT_TYPE_JSON, full_response);
    return 0;
}

// Handle DELETE request - remove resource
int handle_delete_request(int client_fd, const char *path) {
    // For now, just return a success response
    // In a real implementation, you'd actually delete the resource
    const char *response_body = "{\"message\": \"Resource deleted successfully\", \"path\": \"";
    char full_response[1024];
    snprintf(full_response, sizeof(full_response), "%s%s\"}", response_body, path);
    
    send_http_response(client_fd, HTTP_STATUS_204, CONTENT_TYPE_JSON, NULL);
    return 0;
} 