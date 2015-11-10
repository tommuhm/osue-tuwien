CC=gcc
CFLAGS=-Wall -std=c99 -pedantic -D_XOPEN_SOURCE=500 -D_OPEN_BSD

.PHONY: all clean

all: dsort

dsort: dsort.o

$.o: $.c
	$(CC) $(CFLAGS) $^

clean:
	rm -f dsort dsort.o
