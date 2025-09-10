CFLAGS=-Wall -Wextra

ifeq ($(OS),Windows_NT)
    LDLIBS=-lws2_32
else
    LDLIBS=
endif

.PHONY: all
all: 01_basic_server 02_post

01_basic_server: 01_basic_server.c ../thinwire.h
	$(CC) $(CFLAGS) -o 01_basic_server 01_basic_server.c $(LDLIBS)

02_post: 02_post.c ../thinwire.h
	$(CC) $(CFLAGS) -o 02_post 02_post.c $(LDLIBS)