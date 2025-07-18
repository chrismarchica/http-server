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

// Content Types
#define CONTENT_TYPE_HTML "text/html"
#define CONTENT_TYPE_CSS "text/css"
#define CONTENT_TYPE_JS "application/javascript"
#define CONTENT_TYPE_JSON "application/json"
#define CONTENT_TYPE_PLAIN "text/plain"
#define CONTENT_TYPE_PNG "image/png"
#define CONTENT_TYPE_JPG "image/jpeg"
#define CONTENT_TYPE_GIF "image/gif"

// Function declarations
void send_http_response(int client_fd, const char *status, const char *content_type, const char *body);
void send_error_response(int client_fd, const char *status, const char *message);
const char* get_content_type(const char *filename);
int is_supported_method(const char *method);

// HTTP method handlers
int handle_get_request(int client_fd, const char *path);
int handle_post_request(int client_fd, const char *path);
int handle_put_request(int client_fd, const char *path);
int handle_delete_request(int client_fd, const char *path);

#endif 