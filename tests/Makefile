CFLAGS=-Wall -Wextra

ifeq ($(OS),Windows_NT)
    LDLIBS=-lws2_32
else
    LDLIBS=
endif

.PHONY: all
all: tw_map

tw_map: tw_map.c test.c test.h ../thinwire.h
	$(CC) $(CFLAGS) -o tw_map tw_map.c test.c $(LDLIBS)