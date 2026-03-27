CC = gcc
CFLAGS = -Wall -Wextra -std=c11
TARGET = test
SRCS = test.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET) *.o