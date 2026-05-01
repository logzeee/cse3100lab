#include <stdio.h>
#include <unistd.h>

void task(int v) {
  printf("Task is running on value %d\n", v);
  sleep(10);
  printf("Done with task on value %d\n", v);
}

int main() {

  task(1);
  task(2);

  return 0;
}
