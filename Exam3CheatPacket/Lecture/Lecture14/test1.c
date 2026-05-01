#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>


void *compute_sum(void *arg) {
  int n = *(int *)arg;
  int *result = malloc(sizeof(int));
  *result = 0;

  for (int i = 1; i <= n; i++) {
    *result += i;
  }

	/* return (void* ) result; */
	pthread_exit(result);
}

int main() {
  pthread_t thread;
  int n = 10;
  int *result;

  // Create a new thread to execute the compute_sum function
  if (pthread_create(&thread, NULL, compute_sum, &n)) {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }

  // Wait for the thread to finish and capture the result
  if (pthread_join(thread, (void **)&result)) {
    fprintf(stderr, "Error joining thread\n");
    return 2;
  }

  // Print the result returned by the thread
  printf("Sum of first %d natural numbers: %d\n", n, *result);

  // Free the allocated memory for result
  free(result);

  return 0;
}
