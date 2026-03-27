#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>


// redirect standard input to the specified file
void redirectStdin(const char *filename) {
  int fd = open(filename, O_RDONLY);
  if (fd < 0) {
    perror("Error opening the file\n");
    exit(-1);
  }
  // TODO
  // fill in the code below
  dup2(fd, 0);
  close(fd);

}

// redirect standad output to the specified file
void redirectStdout(const char *filename) {
  int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  if (fd < 0) {
    perror("Error opening the file\n");
    exit(-1);
  }
  // TODO
  // fill in the code below
  dup2(fd, 1);
  close(fd);
}
