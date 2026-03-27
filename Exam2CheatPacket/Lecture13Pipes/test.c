#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// Goal: Make parent process have two child processes and
// make child process 1 say hello to child process 2
//
// child process 1 is only writing
// child process 2 is only readin
// parent process is not communicating with any of the children

int main() {
  int p[2];
  pipe(p);

  int pid1 = fork();
  if (pid1 == 0) {
    // child process 1
    // This child process will say hello
    // So, it will only write and doesn't read
    close(p[0]); // closing the read end of the pipe
    write(p[1], "hello", 5);
    close(p[1]);
    printf("Child process 1 wrote hello\n");
    exit(0);
  }
	// parent process

  close(p[1]);


  int pid2 = fork();
  if (pid2 == 0) {
    // child process 2
    // This child process will read hello
    // so, it will only read and doesn't write
    /* close(p[1]); // closing write end of the pipe */
    char buf;
    printf("Child process 2 read:");
    while (read(p[0], &buf, 1) > 0) {
      printf("%c", buf);
    }
    printf("\n");
    close(p[0]);
    exit(0);
  }
  // parent process
  // parent process doesn't communicate with any of the child processes
  close(p[0]);

  return 0;
}
