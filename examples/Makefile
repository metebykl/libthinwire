CFLAGS=-Wall -Wextra

ifeq ($(OS),Windows_NT)
    LDLIBS=-lws2_32
else
    LDLIBS=
endif

.PHONY: all
all: 01_basic_server

01_basic_server: 01_basic_server.c ../thinwire.h
	$(CC) $(CFLAGS) -o 01_basic_server 01_basic_server.c $(LDLIBS)