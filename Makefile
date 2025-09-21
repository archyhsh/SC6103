CC = gcc
CFLAGS = -Wall -g -Iudp -Ibackend    # -Iudp 表示头文件在 udp/ 目录
SRC = main.c udp/marshalize.c udp/hash_function.c udp/demarshalize.c udp/at_most_once.c udp/udp_server.c backend/db_init.c backend/operate.c backend/parseTime.c
OBJ = $(SRC:.c=.o)
TARGET = main
LDLIBS = -lsqlite3

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDLIBS)

clean:
	rm -f $(OBJ) $(TARGET)