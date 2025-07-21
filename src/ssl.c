#include "utils/ssl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>

// Initialize SSL context
int init_ssl(ssl_config_t *config) {
    if (!config) return -1;
    
    // Initialize OpenSSL
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    
    // Create SSL context with more compatible settings
    config->ctx = SSL_CTX_new(TLS_server_method());
    if (!config->ctx) {
        fprintf(stderr, "Failed to create SSL context\n");
        return -1;
    }
    
    // Set SSL options for better compatibility
    SSL_CTX_set_options(config->ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3);
    SSL_CTX_set_min_proto_version(config->ctx, TLS1_VERSION);
    SSL_CTX_set_max_proto_version(config->ctx, TLS1_3_VERSION);
    
    // Set cipher list for better compatibility
    SSL_CTX_set_cipher_list(config->ctx, "HIGH:!aNULL:!MD5:!RC4");
    
    // Load certificate and private key
    if (SSL_CTX_use_certificate_file(config->ctx, config->cert_file, SSL_FILETYPE_PEM) <= 0) {
        fprintf(stderr, "Failed to load certificate file: %s\n", config->cert_file);
        SSL_CTX_free(config->ctx);
        return -1;
    }
    
    if (SSL_CTX_use_PrivateKey_file(config->ctx, config->key_file, SSL_FILETYPE_PEM) <= 0) {
        fprintf(stderr, "Failed to load private key file: %s\n", config->key_file);
        SSL_CTX_free(config->ctx);
        return -1;
    }
    
    // Verify private key
    if (SSL_CTX_check_private_key(config->ctx) <= 0) {
        fprintf(stderr, "Private key does not match certificate\n");
        SSL_CTX_free(config->ctx);
        return -1;
    }
    
    config->ssl_enabled = 1;
    printf("SSL initialized successfully\n");
    return 0;
}

// Cleanup SSL resources
void cleanup_ssl(ssl_config_t *config) {
    if (config && config->ctx) {
        SSL_CTX_free(config->ctx);
        config->ctx = NULL;
        config->ssl_enabled = 0;
    }
    EVP_cleanup();
}

// Create SSL connection for a client
SSL *create_ssl_connection(SSL_CTX *ctx, int client_fd) {
    SSL *ssl = SSL_new(ctx);
    if (!ssl) {
        fprintf(stderr, "Failed to create SSL connection\n");
        return NULL;
    }
    
    SSL_set_fd(ssl, client_fd);
    
    // Set SSL mode for better compatibility
    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
    
    // Set a timeout for the SSL handshake
    struct timeval timeout;
    timeout.tv_sec = 10;  // 10 second timeout
    timeout.tv_usec = 0;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    printf("Attempting SSL handshake...\n");
    int ret = SSL_accept(ssl);
    if (ret <= 0) {
        int err = SSL_get_error(ssl, ret);
        
        // Check if this is a timeout or connection issue
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            // This might be a timeout or the client isn't sending SSL data
            fprintf(stderr, "SSL handshake timeout or incomplete - client may be using HTTP\n");
        } else {
            char err_buf[256];
            ERR_error_string_n(err, err_buf, sizeof(err_buf));
            fprintf(stderr, "SSL accept failed: %d - %s\n", err, err_buf);
            
            // Try to get more specific error info
            switch (err) {
                case SSL_ERROR_ZERO_RETURN:
                    fprintf(stderr, "SSL: Connection closed by peer\n");
                    break;
                case SSL_ERROR_SYSCALL:
                    fprintf(stderr, "SSL: System call error\n");
                    break;
                case SSL_ERROR_SSL:
                    fprintf(stderr, "SSL: SSL protocol error\n");
                    break;
                default:
                    fprintf(stderr, "SSL: Unknown error\n");
                    break;
            }
        }
        
        SSL_free(ssl);
        return NULL;
    }
    
    printf("SSL connection established successfully\n");
    return ssl;
}

// Close SSL connection
void close_ssl_connection(SSL *ssl) {
    if (ssl) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }
}

// SSL read wrapper
int ssl_read(SSL *ssl, char *buffer, int size) {
    return SSL_read(ssl, buffer, size);
}

// SSL write wrapper
int ssl_write(SSL *ssl, const char *data, int size) {
    return SSL_write(ssl, data, size);
}

// Generate self-signed certificate for development
int generate_self_signed_cert(const char *cert_file, const char *key_file) {
    char cmd[512];
    
    // Check if files already exist
    struct stat st;
    if (stat(cert_file, &st) == 0 && stat(key_file, &st) == 0) {
        printf("Certificate files already exist\n");
        return 0;
    }
    
    // Generate private key
    snprintf(cmd, sizeof(cmd), 
             "openssl genrsa -out %s 2048 2>/dev/null", key_file);
    if (system(cmd) != 0) {
        fprintf(stderr, "Failed to generate private key\n");
        return -1;
    }
    
    // Generate self-signed certificate
    snprintf(cmd, sizeof(cmd),
             "openssl req -new -x509 -key %s -out %s -days 365 -subj "
             "'/C=US/ST=State/L=City/O=Organization/CN=localhost' 2>/dev/null",
             key_file, cert_file);
    if (system(cmd) != 0) {
        fprintf(stderr, "Failed to generate certificate\n");
        unlink(key_file); // Clean up key file
        return -1;
    }
    
    printf("Self-signed certificate generated: %s\n", cert_file);
    printf("Private key generated: %s\n", key_file);
    return 0;
} 