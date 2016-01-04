##
## makefile for battleships
##
## @author Thomas Muhm 1326486
##
## @date 04.01.2016
##
##

CC=gcc
CFLAGS=-Wall -std=c99 -pedantic -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -g

.PHONY: all clean

all: battleships

battleships: battleships.o

$.o: $.c
	$(CC) $(CFLAGS) $^

clean:
	rm -f battleships battleships.o

debug: CFLAGS += -DENDEBUG
debug: all
