#define more variables so it is easier to make changes
CC=gcc
CFLAGS=-g -Wall
TARGETS=fork-puzzle

all: $(TARGETS)

fork-puzzle: fork-puzzle.c
	$(CC) $(CFLAGS) -o $@ $@.c

clean:
	rm -rf *.o *~ $(TARGETS)
