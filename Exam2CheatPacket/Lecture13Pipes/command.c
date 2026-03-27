#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// goal: cat doc.txt | tr a-z A-Z
//
//

int main() {

  int p[2];
  int ret = pipe(p);
  if (ret == -1) {
    perror("pipe failed\n");
  }

  int pid1 = fork();
  if (pid1 == 0) {
    close(p[0]);
    dup2(p[1], 1);
    close(p[1]);
    execlp("cat", "cat", "pipe.txt", NULL);
    exit(1);
  }

  int pid2 = fork();
  if (pid2 == 0) {
    close(p[1]);
    dup2(p[0], 0);
    close(p[0]);
    execlp("tr", "tr", "a-z", "A-Z", NULL);
    exit(0);
  }

  close(p[0]);
  close(p[1]);

  waitpid(pid1, NULL, 0);
  waitpid(pid2, NULL, 0);

  return 0;
}
