CC = gcc
CFLAGS = -Wall -g -Iudp    # -Iudp 表示头文件在 udp/ 目录
SRC = main.c udp/marshalize.c udp/hash_function.c udp/demarshalize.c udp/at_most_once.c udp/udp_client.c
OBJ = $(SRC:.c=.o)
TARGET = main
LDLIBS = -lsqlite3

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDLIBS)

clean:
	rm -f $(OBJ) $(TARGET)