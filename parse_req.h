#ifndef REQUEST_H
#define REQUEST_H

int parse_request(const char *buffer, char *method, size_t msize, char *path, size_t psize);

#endif
