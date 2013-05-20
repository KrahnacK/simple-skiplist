CC=gcc
CFLAGS=-Wall -Werror -std=c99
CFLAGS+= -g3 -ggdb3 

all: test

.PHONY: all clean

test: test.o skiplist.o
	$(CC) $^ -o $@

test.o: test.c
	$(CC) $(CFLAGS)	-c $< -o $@

skiplist.o: skiplist.c skiplist.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o test
