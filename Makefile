# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g

# Output binary
OUT = server

# Source files
SRC = main.c server.c parse_req.c

# Object files (optional for better control)
OBJ = main.o server.o parse_req.o

# Default target
all: $(OUT)

$(OUT): $(OBJ)
	$(CC) $(CFLAGS) -o $(OUT) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OUT) *.o

