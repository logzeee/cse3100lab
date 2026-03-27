#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

// Goal: child process will send an integer to the parent process
// child process will communicate with parent process and send the parent
// process an integer

int main() {

  int p[2];
  pipe(p);

  int pid = fork();

  if (pid == 0) {
    // child process
    // child process will only write, it doesn't read
    close(p[0]);
    printf("This is a child process\n");
    int value = 369;
    write(p[1], &value, sizeof(int));
    close(p[1]);
    printf("Child process wrote  = %d\n", value);
    exit(0);
  }
  // parent process
  // parent process will only read , it doesn't write
  close(p[1]);
  int value;
  read(p[0], &value, sizeof(int));
  close(p[0]);
  printf("parent process read = %d\n", value);
  return 0;
}
