CC = gcc
DEBUG = -g -ggdb
CFLAGS = -O2 -std=c99 -fPIC -Wall -Werror $(DEBUG)

LIB_OBJS = \
	./lib/chained_key.o \
	./lib/djb2.o \
	./lib/hash_table.o

SHARED_LIB = ./libhashtable.so

$(SHARED_LIB): $(LIB_OBJS)
	$(CC) $(DEBUG) -shared -Wl,--export-dynamic $(LIB_OBJS) -o $(SHARED_LIB)

hash_table_test:
	$(CC) $(DEBUG) -o hash_table_test hash_table_test.c $(SHARED_LIB)

server:
	$(CC) -pthread $(DEBUG) -o server server.c $(SHARED_LIB)

clean:
	rm -f ./lib/*.o
	rm -f ./libhashtable.so
	rm -f ./hash_table_test
	rm -f ./server

.PHONY: clean hash_table_test server
