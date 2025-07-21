#ifndef HTTP_H
#define HTTP_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>

// HTTP Methods
#define HTTP_METHOD_GET "GET"
#define HTTP_METHOD_POST "POST"
#define HTTP_METHOD_PUT "PUT"
#define HTTP_METHOD_DELETE "DELETE"

// HTTP Status Codes
#define HTTP_STATUS_200 "HTTP/1.1 200 OK"
#define HTTP_STATUS_201 "HTTP/1.1 201 Created"
#define HTTP_STATUS_204 "HTTP/1.1 204 No Content"
#define HTTP_STATUS_403 "HTTP/1.1 403 Forbidden"
#define HTTP_STATUS_400 "HTTP/1.1 400 Bad Request"
#define HTTP_STATUS_404 "HTTP/1.1 404 Not Found"
#define HTTP_STATUS_405 "HTTP/1.1 405 Method Not Allowed"
#define HTTP_STATUS_500 "HTTP/1.1 500 Internal Server Error"
#define HTTP_STATUS_429 "HTTP/1.1 429 Too Many Requests"

// Content Types
#define CONTENT_TYPE_HTML "text/html"
#define CONTENT_TYPE_CSS "text/css"
#define CONTENT_TYPE_JS "application/javascript"
#define CONTENT_TYPE_JSON "application/json"
#define CONTENT_TYPE_PLAIN "text/plain"
#define CONTENT_TYPE_PNG "image/png"
#define CONTENT_TYPE_JPG "image/jpeg"
#define CONTENT_TYPE_GIF "image/gif"

// Request structure
typedef struct {
    char method[16];
    char path[256];
    char query_string[512];
    char headers[2048];
    char body[4096];
    int content_length;
} http_request_t;

// Response structure
typedef struct {
    char status[64];
    char headers[2048];
    char body[8192];
    int content_length;
} http_response_t;

// Function declarations
void send_http_response(int client_fd, const char *status, const char *content_type, const char *body);
void send_http_response_ssl(void *ssl, const char *status, const char *content_type, const char *body);
void send_error_response(int client_fd, const char *status, const char *message);
void send_error_response_ssl(void *ssl, const char *status, const char *message);
const char* get_content_type(const char *filename);
int is_supported_method(const char *method);

// HTTP method handlers
int handle_get_request(int client_fd, void *ssl, const char *path);
int handle_post_request(int client_fd, void *ssl, const char *path);
int handle_put_request(int client_fd, void *ssl, const char *path);
int handle_delete_request(int client_fd, void *ssl, const char *path);

// New advanced features
int parse_http_request(const char *buffer, http_request_t *req);
void send_json_response(int client_fd, void *ssl, const char *status, const char *json_data);
void send_cors_headers(int client_fd);
int parse_query_string(const char *query, char **params, int max_params);
void add_response_header(http_response_t *resp, const char *name, const char *value);
void send_full_response(int client_fd, http_response_t *resp);

// API endpoints
int handle_api_users(int client_fd, void *ssl, const char *method, const char *path, const http_request_t *req);
int handle_api_health(int client_fd, void *ssl);
int handle_api_metrics(int client_fd, void *ssl);

#endif 