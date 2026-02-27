CC=gcc
CFLAGS=-Wall -g -std=c99
TARGETS=ziplist common

all: $(TARGETS)

$(TARGETS): %: %.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f *.o *~ $(TARGETS) a.out
