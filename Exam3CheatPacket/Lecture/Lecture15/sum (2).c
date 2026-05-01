#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define N 10

int arr[N] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
long partial_sum[2]; 

void *sum_array(void *arg) {
  int thread_id = *(int *)arg;
  int start = thread_id * (N / 2);
  int end = (thread_id + 1) * (N / 2);
  long sum = 0;

  for (int i = start; i < end; i++)
    sum += arr[i];

  partial_sum[thread_id] = sum;
  pthread_exit(NULL);
}

int main() {
  pthread_t threads[2];
  int thread_id[2];

  // Create two threads
  for (int i = 0; i < 2; i++) {
    thread_id[i] = i;
    pthread_create(&threads[i], NULL, sum_array, &thread_id[i]);
  }

  // Wait for both threads
  for (int i = 0; i < 2; i++) {
    pthread_join(threads[i], NULL);
  }

  // Combine results
  long total = partial_sum[0] + partial_sum[1];
  printf("Total sum = %ld\n", total);

  return 0;
}
