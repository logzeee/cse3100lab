#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE 1 // Size of the buffer

int buffer[BUFFER_SIZE];
int count = 0; // Number of items in the buffer

pthread_mutex_t mutex;
pthread_cond_t cond_producer;
pthread_cond_t cond_consumer;

void initialize_sync_objects() {
  // Initialize the mutex with default attributes
  if (pthread_mutex_init(&mutex, NULL) != 0) {
    perror("Mutex initialization failed");
    exit(EXIT_FAILURE);
  }

  // Initialize the condition variable for the producer with default attributes
  if (pthread_cond_init(&cond_producer, NULL) != 0) {
    perror("Producer condition variable initialization failed");
    exit(EXIT_FAILURE);
  }

  // Initialize the condition variable for the consumer with default attributes
  if (pthread_cond_init(&cond_consumer, NULL) != 0) {
    perror("Consumer condition variable initialization failed");
    exit(EXIT_FAILURE);
  }
}

void cleanup_sync_objects() {
  // Destroy the mutex and condition variables
  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond_producer);
  pthread_cond_destroy(&cond_consumer);
}

void *producer(void *arg) {
  int item;
  while (1) {
    item = rand() % 100; // Produce a random item
    pthread_mutex_lock(&mutex);

    // Wait until there's space in the buffer
    while (count == BUFFER_SIZE) {
      pthread_cond_wait(&cond_producer, &mutex);
    }

    // Add item to buffer
    buffer[count] = item;
    count++;
    printf("Produced: %d\n", item);

    // Signal the consumer that there's a new item
    pthread_cond_signal(&cond_consumer);
    pthread_mutex_unlock(&mutex);

    sleep(1); // Sleep for a while to simulate production time
  }
}

void *consumer(void *arg) {
  int item;
  while (1) {
    pthread_mutex_lock(&mutex);

    // Wait until there's an item in the buffer
    while (count == 0) {
			printf("consumer enters while loop\n");
      pthread_cond_wait(&cond_consumer, &mutex);
			printf("signal\n");
    }

    // Remove item from buffer
    item = buffer[count - 1];
    count--;
    printf("Consumed: %d\n", item);

    // Signal the producer that there's space in the buffer
    pthread_cond_signal(&cond_producer);
    pthread_mutex_unlock(&mutex);

    /* sleep(1); // Sleep for a while to simulate consumption time */
  }
}

int main() {
  pthread_t prod, cons;

  // Initialize mutex and condition variables
  initialize_sync_objects();

  // Create producer and consumer threads
  pthread_create(&prod, NULL, producer, NULL);
  pthread_create(&cons, NULL, consumer, NULL);

  // Wait for threads to finish (they won't in this example)
  pthread_join(prod, NULL);
  pthread_join(cons, NULL);

  // Clean up mutex and condition variables
  cleanup_sync_objects();

  return 0;
}
