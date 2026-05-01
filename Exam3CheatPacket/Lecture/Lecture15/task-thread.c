#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

void *task(void *arg) {
  int *v = (int *)arg;
  printf("Task is running on value %d\n", *v);
  sleep(10);
  printf("Done with task on value %d\n", *v);
  pthread_exit(NULL);
}

int main() {

  pthread_t t1, t2;
  int v1 = 1;
  int v2 = 2;
  pthread_create(&t1, NULL, task, &v1);
  pthread_create(&t2, NULL, task, &v2);


	pthread_join(t1,NULL);
	pthread_join(t2,NULL);
  return 0;
}
