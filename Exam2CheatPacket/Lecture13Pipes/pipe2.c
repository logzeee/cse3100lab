#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// goal: create two child processes, make child process 1 send hello to child
// process 2
//  child process 1 is only writing, it doesn't read
//  child process 2 is only reading, it doesn't write
//  parent process is not communicating with any child processes

int main() {
  int p[2];
  pipe(p);

  int pid1 = fork();
  if (pid1 == 0) {
    // child process 1
    // will only write
    // doesn't read
    close(p[0]);
    write(p[1], "hello", 5);
    close(p[1]);
    printf("Child process 1 is writing hello\n");

    exit(0);
  }

  int pid2 = fork();
  if (pid2 == 0) {
    // child process 2
    // will only  read
    // doesn't write
    close(p[1]);
    char buf;
    printf("child process 2 is reading:");
    while (read(p[0], &buf, 1) > 0) {
      printf("%c", buf);
    }
    printf("\n");
    close(p[0]);
    exit(0);
  }

  // parent process
  close(p[0]);
  close(p[1]);
  return 0;
}
