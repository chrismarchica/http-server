# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g

# Output binary name
OUT = server

# Source files
SRC = main.c server.c client.c parse_req.c

# Object files (built from source files)
OBJ = $(SRC:.c=.o)

# Default rule
all: $(OUT)

# Linking final binary
$(OUT): $(OBJ)
	$(CC) $(CFLAGS) -o $(OUT) $(OBJ) -lpthread

# Compile each .c into .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Cleanup rule
clean:
	rm -f *.o $(OUT)

