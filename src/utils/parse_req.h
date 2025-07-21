#ifndef REQUEST_H
#define REQUEST_H

#include "http.h"

// Enhanced request parsing
int parse_request(const char *buffer, char *method, size_t msize, char *path, size_t psize);
int parse_full_request(const char *buffer, http_request_t *req);
int extract_path_and_query(const char *uri, char *path, size_t path_size, char *query, size_t query_size);
int parse_headers(const char *header_section, char *headers, size_t header_size);

#endif
