// Parent process will send an integer n
// Child process will return sum of n natural numbers
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
// in child process
// child process will recieve value n
// it will then compute sum of n natural number
// it will write the result to parent

// parent process
// parent will send /write value n
// it will recieve the sum of n natural numbers from child

int main() {

  int pc[2]; // use for parent(write)---> child(read) pc[1] ---> pc[0];
  int cp[2]; // use for child(write)---> parent (read) cp[1] ---> cp[0];

  pipe(pc);
  pipe(cp);

  int pid = fork();
  if (pid == 0) {
    // child process
    // child process will first read from pc[0], read value n, it will never
    // write to pc[1];
    close(pc[1]);
    int n;
    read(pc[0], &n, sizeof(int));
    printf("n = %d\n", n);
    close(pc[0]);
    int res = (n * (n + 1)) / 2;
    // child process will write / send result to parent, it will write to cp[1],
    // it never reads from cp[0];
    close(cp[0]);
    write(cp[1], &res, sizeof(int));
    close(cp[1]);
    exit(0);
  }
  // parent process
  // write / send value n to child, it writes to pc[1], it never reads from
  // pc[0]
  close(pc[0]);
  int n = 5;
  write(pc[1], &n, sizeof(int));
  close(pc[1]);
  int res;
  // reads from child, reads from cp[0], it never writes to cp[1];
  close(cp[1]);
  read(cp[0], &res, sizeof(int));
  close(cp[0]);
  printf("result = %d\n", res);
  return 0;
}
