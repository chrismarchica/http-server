#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "utils/parse_req.h"

int parse_request(const char *buffer, char *method, size_t msize, char *path, size_t psize)
{   
    char fmt[32];
    snprintf(fmt, sizeof(fmt), "%%%zus %%%zus", msize-1, psize-1);
    return sscanf(buffer, fmt, method, path) == 2;
}

int parse_full_request(const char *buffer, http_request_t *req) {
    if (!buffer || !req) return 0;
    
    // Clear the request structure
    memset(req, 0, sizeof(http_request_t));
    
    // Find the end of the first line (request line)
    const char *end_of_line = strstr(buffer, "\r\n");
    if (!end_of_line) return 0;
    
    // Parse request line: METHOD URI HTTP_VERSION
    char uri[512];
    char http_version[32];
    
    if (sscanf(buffer, "%15s %511s %31s", req->method, uri, http_version) != 3) {
        return 0;
    }
    
    // Extract path and query string from URI
    if (!extract_path_and_query(uri, req->path, sizeof(req->path), 
                               req->query_string, sizeof(req->query_string))) {
        return 0;
    }
    
    // Find headers section
    const char *headers_start = end_of_line + 2;
    const char *body_start = strstr(headers_start, "\r\n\r\n");
    
    if (body_start) {
        // Parse headers
        size_t headers_len = body_start - headers_start;
        if (headers_len < sizeof(req->headers)) {
            strncpy(req->headers, headers_start, headers_len);
            req->headers[headers_len] = '\0';
        }
        
        // Parse body
        const char *body = body_start + 4;
        size_t body_len = strlen(body);
        if (body_len < sizeof(req->body)) {
            strncpy(req->body, body, body_len);
            req->content_length = body_len;
        }
    } else {
        // No body, just headers
        strncpy(req->headers, headers_start, sizeof(req->headers) - 1);
    }
    
    return 1;
}

int extract_path_and_query(const char *uri, char *path, size_t path_size, char *query, size_t query_size) {
    if (!uri || !path || !query) return 0;
    
    const char *query_start = strchr(uri, '?');
    
    if (query_start) {
        // Has query string
        size_t path_len = query_start - uri;
        if (path_len >= path_size) return 0;
        
        strncpy(path, uri, path_len);
        path[path_len] = '\0';
        
        const char *query_str = query_start + 1;
        size_t query_len = strlen(query_str);
        if (query_len >= query_size) return 0;
        
        strncpy(query, query_str, query_len);
        query[query_len] = '\0';
    } else {
        // No query string
        size_t uri_len = strlen(uri);
        if (uri_len >= path_size) return 0;
        
        strncpy(path, uri, uri_len);
        path[uri_len] = '\0';
        query[0] = '\0';
    }
    
    return 1;
}

int parse_headers(const char *header_section, char *headers, size_t header_size) {
    if (!header_section || !headers) return 0;
    
    size_t len = strlen(header_section);
    if (len >= header_size) return 0;
    
    strncpy(headers, header_section, len);
    headers[len] = '\0';
    
    return 1;
}
