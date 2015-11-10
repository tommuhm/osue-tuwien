##
## makefile for dsort
##
## @author Thomas Muhm 1326486
##
## @date 10.11.2015
##
##

CC=gcc
CFLAGS=-Wall -std=c99 -pedantic -D_XOPEN_SOURCE=500 -D_OPEN_BSD -g

.PHONY: all clean

all: dsort

dsort: dsort.o

$.o: $.c
	$(CC) $(CFLAGS) $^

clean:
	rm -f dsort dsort.o

debug: CFLAGS += -DENDEBUG
debug: all
