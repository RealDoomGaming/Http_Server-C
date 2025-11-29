CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -IdataStructures -IThpool -IServer -g

SRC = \
	DataStructures/queue.c \
	Thpool/thpool.c \
	Server/server.c

OBJ = $(SRC:.c=.o)

TARGET = serverapp

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Generic .c → .o rule
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)


