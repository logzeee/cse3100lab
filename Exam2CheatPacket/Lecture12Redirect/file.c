#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int printLine(int fd) {
	// reads a file provided by fd and prints the line read to stdout
  int count = 0;
  int n;
  char c;

  while ((n = read(fd, &c, 1)) > 0) {
    count += n;
    if (c == '\n')
      break;
    printf("%c", c);
  }
  printf("\n");
  return count;
}

int writeLine(int fd, const char *buf) {
	// writes string stored in buf to file provided by fd
  if (!buf)
    return -1;

  int len = strlen(buf);
  int total = 0;
  int n;

  while (total < (ssize_t)len) {
    n = write(fd, buf + total, len - total);
    if (n < 0)
      return -1;
    total += n;
  }

  do {
    n = write(fd, "\n", 1);
  } while (n < 0 && errno == EINTR);
  if (n < 0)
    return -1;

  return total + 1;
}

int main() {
  int fd = open("doc.txt", O_RDWR | O_APPEND);
	/* close(1); */
	/* int duplicate_fd = dup(fd); */

	int dup2_fd = dup2(fd,1);
	printf("fd = %d,duplicate_fd = %d\n",fd,dup2_fd);

  printLine(fd);
  writeLine(fd, "Hello");

  printf("This is a print statement\n");// will print to stdout
  fprintf(stderr, "This is being written to stderr\n");

  close(fd);
	/* close(duplicate_fd); */
  return 0;
}
