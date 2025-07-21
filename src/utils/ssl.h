#ifndef SSL_H
#define SSL_H

#ifdef USE_SSL
#include <openssl/ssl.h>
#include <openssl/err.h>

// SSL context and configuration
typedef struct {
    SSL_CTX *ctx;
    int ssl_enabled;
    char *cert_file;
    char *key_file;
} ssl_config_t;

// Function declarations
int init_ssl(ssl_config_t *config);
void cleanup_ssl(ssl_config_t *config);
SSL *create_ssl_connection(SSL_CTX *ctx, int client_fd);
void close_ssl_connection(SSL *ssl);
int ssl_read(SSL *ssl, char *buffer, int size);
int ssl_write(SSL *ssl, const char *data, int size);

// Certificate generation (for development)
int generate_self_signed_cert(const char *cert_file, const char *key_file);

#else
// Dummy types and functions when SSL is not available
typedef struct {
    void *ctx;
    int ssl_enabled;
    char *cert_file;
    char *key_file;
} ssl_config_t;

typedef void SSL;

// Dummy function declarations
#define init_ssl(config) (-1)
#define cleanup_ssl(config) 
#define create_ssl_connection(ctx, client_fd) (NULL)
#define close_ssl_connection(ssl)
#define ssl_read(ssl, buffer, size) (-1)
#define ssl_write(ssl, data, size) (-1)
#define generate_self_signed_cert(cert_file, key_file) (-1)

#endif

#endif 