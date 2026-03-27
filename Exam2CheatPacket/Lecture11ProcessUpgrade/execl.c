#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
  printf("This file will run some other c file\n");
  int a = fork();
  if (a == 0) {
    execl("./calc", "./calc", "sum", "1", "2", "3", "4", "NULL");
    printf("If you can see this, then exec failed\n");
  } else {
    wait(NULL);
    printf("Done with that other file\n");
  }

  return 0;
}
