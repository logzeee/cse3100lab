#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char **argv) {

  printf("This file will run some other c file\n");
  int a = fork();
  if (a == 0) {
    /* execl("./videos/calc", "./calc", "sum", "1", "2", "3", "4", NULL); */

    /* execlp("ls","ls","-l",NULL); */

    /* char *args[] = {"./calc", "sum", "1", "2", "3", "4", NULL}; */

    /* execv("./videos/calc", &argv[1]); */

    /* char *args[] = {"ls", "-l", NULL}; */
    execvp(argv[1], &argv[1]);

    printf("If you can see this, then exec failed\n");
  } else {
    wait(NULL);
    printf("Done with that other file\n");
  }

  return 0;
}
