#
# makefile for mydiff
#
# @author Thomas Muhm 1326486
#

CC=gcc
DEFS=-D_XOPEN_SOURCE=500 -D_BSD_SOURCE
CFLAGS=-Wall -g -std=c99 -pedantic $(DEFS)

OBJECTFILES=server.o client.o

.PHONY: all clean

all: server client

server: server.o
	$(CC) $(LDFLAGS) -o $@ $^

client: client.o
	$(CC) $(LDFLAGS) -o $@ $^

$.o: $.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTFILES) server client

debug: CFLAGS += -DENDEBUG
debug: all
