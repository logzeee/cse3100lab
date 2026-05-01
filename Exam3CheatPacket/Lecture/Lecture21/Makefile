TARGETS=server client
SRCS=$(patsubst %,%.c,$(TARGETS))
CC=gcc
CFLAGS= -Wall -g -pthread -Wextra

all : $(TARGETS)

$(TARGETS): %: %.c socketio.c
	$(CC) $(CFLAGS) -o $@ $<

clean: 
	@rm -f $(TARGETS) *.o a.out 

sub: 
	@zip $${PWD##*/}.zip $(SRCS) Makefile
