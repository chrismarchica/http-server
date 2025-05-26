
CC = gcc
CFLAGS = -Wall -Wextra -g

# Source and output
SRC = main.c server.c
OUT = server


all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)


clean:
	rm -f $(OUT)

