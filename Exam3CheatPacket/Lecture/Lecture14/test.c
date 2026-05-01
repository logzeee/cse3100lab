#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

// Function to be executed by the thread
void *print_message(void *arg) {
  char *message = (char *)arg;
  printf("%s\n", message);
  pthread_exit(NULL); // Exit the thread
}

int main() {
  pthread_t thread; // Thread identifier
  char *message = "Hello from the thread!";

  // Create a new thread to execute the print_message function
  if (pthread_create(&thread, NULL, print_message, (void *)message)) {
    fprintf(stderr, "Error creating thread\n");
    return 1;
  }

  // Wait for the thread to complete
  if (pthread_join(thread, NULL)) {
    fprintf(stderr, "Error joining thread\n");
    return 2;
  }

  printf("Thread has finished execution.\n");

  return 0;
}
