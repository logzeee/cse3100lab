
#include <stdio.h>
#include <unistd.h>

int main() {
  int a = fork();
  if (a == 0) {
    printf("This is in child,PID:%d\n", getpid());
    while (1) {
    }
  } else {

    printf("This is in parent,PID:%d\n", getpid());
    while (1) {
    }
  }
}
