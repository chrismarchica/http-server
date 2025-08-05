# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g

# Output binary name
OUT = server

# Source files
SRC = src/main.c src/server.c src/client.c src/parse_req.c src/http.c src/api.c

# Check if OpenSSL is available (with fallback for systems without pkg-config)
OPENSSL_AVAILABLE := $(shell (pkg-config --exists openssl 2>/dev/null && echo "yes") || (echo "#include <openssl/ssl.h>" | gcc -E - >/dev/null 2>&1 && echo "yes") || echo "no")

ifeq ($(OPENSSL_AVAILABLE),yes)
    CFLAGS += -DUSE_SSL
    SRC += src/ssl.c
    LIBS = -lpthread -lssl -lcrypto
    SSL_INFO = "with HTTPS support"
else
    LIBS = -lpthread
    SSL_INFO = "HTTP only - OpenSSL not available"
endif

# Object files (built from source files)
OBJ = $(SRC:.c=.o)

# Default rule
all: $(OUT)
	@echo "Built $(OUT) $(SSL_INFO)"

# Linking final binary
$(OUT): $(OBJ)
	$(CC) $(CFLAGS) -o $(OUT) $(OBJ) $(LIBS)

# Compile each .c into .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile source files in src/ directory
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Cleanup rule
clean:
	rm -f *.o src/*.o $(OUT)

# Install OpenSSL dependencies (Ubuntu/Debian)
install-deps:
	@echo "Installing OpenSSL development libraries..."
	sudo apt update && sudo apt install -y openssl libssl-dev

# Install OpenSSL dependencies (CentOS/RHEL)
install-deps-rhel:
	@echo "Installing OpenSSL development libraries..."
	sudo yum install -y openssl openssl-devel

# Show build info
info:
	@echo "OpenSSL available: $(OPENSSL_AVAILABLE)"
	@echo "Build will include: $(SSL_INFO)"