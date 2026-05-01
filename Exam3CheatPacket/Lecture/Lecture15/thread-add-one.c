#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX 1000000000
int a[MAX];

struct thread_data {
  int thread_num;
  int i, j; // the staring and ending index
};

void *thread_add_one(void *threadarg) {
  struct thread_data *my_data = (struct thread_data *)threadarg;
  for (int k = my_data->i; k <= my_data->j; k++) {
    a[k]++;
  }
  pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: %s n NUM_THREADS\n", argv[0]);
    exit(-1);
  }
  int n = atoi(argv[1]);
  assert(n >= 1 && n <= MAX);
  int NUM_THREADS = atoi(argv[2]);
  assert(NUM_THREADS >= 1 && NUM_THREADS <= 10);

  srand(12345);
  for (int i = 0; i < n; i++) {
    a[i] = rand();
  }
  pthread_t threads[NUM_THREADS];
  struct thread_data thread_data_array[NUM_THREADS];
  int rc, t;
  for (t = 0; t < NUM_THREADS; t++) {
    thread_data_array[t].thread_num = t;
    thread_data_array[t].i = t * n / NUM_THREADS;
    thread_data_array[t].j = (t + 1) * n / NUM_THREADS - 1;
    rc = pthread_create(&threads[t], NULL, thread_add_one,
                        &thread_data_array[t]);
    if (rc) {
      printf("ERROR; return code from pthread_create() is %d\n", rc);
      exit(-1);
    }
  }
  for (t = 0; t < NUM_THREADS; t++) {
    rc = pthread_join(threads[t], NULL);
    if (rc) {
      printf("ERROR; return code from pthread_join() is %d\n", rc);
      exit(-1);
    }
  }
  return 0;
}
