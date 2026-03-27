#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main() {
  int fd = open("test.txt", O_RDWR | O_APPEND);

  char buf[100];
  read(fd, buf, 10);
  printf("%s\n", buf);

  /* read(fd, buf, 10); */
  /* printf("%s\n", buf); */

  /* char str[] = "Hello\n"; */
  /* int v = write(fd, str, strlen(str)); */
  /* printf("%d\n", v); */
  int fd2 = open("test.c",O_RDWR|O_CREAT,0754);
  close(fd);
  close(fd2);
}
