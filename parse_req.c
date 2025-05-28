#include <stdio.h>
#include <stdlib.h>

int parse_request(const char *buffer, char *method, size_t msize, char *path, size_t psize)
{   

    char fmt[32];
    snprintf(fmt, sizeof(fmt), "%%%zus %%%zus", msize-1, psize-1);
    return sscanf(buffer, fmt,method,path) == 2;
}
