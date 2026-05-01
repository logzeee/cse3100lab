#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE 4

int buffer[BUFFER_SIZE];
int count = 0;

pthread_mutex_t mutex;
pthread_cond_t cond_producer;
pthread_cond_t cond_consumer;

void initialize_sync_objects() {
  if (pthread_mutex_init(&mutex, NULL) != 0) {
    perror("Mutex initialization failed");
    exit(EXIT_FAILURE);
  }
  if (pthread_cond_init(&cond_producer, NULL) != 0) {
    perror("Producer condition variable initialization failed");
    exit(EXIT_FAILURE);
  }
  if (pthread_cond_init(&cond_consumer, NULL) != 0) {
    perror("Consumer condition variable initialization failed");
    exit(EXIT_FAILURE);
  }
}

void cleanup_sync_objects() {
  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond_producer);
  pthread_cond_destroy(&cond_consumer);
}

void print_buffer() {
  printf("  Buffer state: [");
  for (int i = 0; i < BUFFER_SIZE; i++) {
    if (i < count)
      printf("%d", buffer[i]);
    else
      printf("_");
    if (i < BUFFER_SIZE - 1) printf("|");
  }
  printf("] count=%d\n", count);
}

void *producer(void *arg) {
  int item;
  while (1) {
    item = rand() % 100;
    pthread_mutex_lock(&mutex);

    while (count == BUFFER_SIZE) {
      printf("Producer waiting — buffer full\n");
      pthread_cond_wait(&cond_producer, &mutex);
    }

    buffer[count] = item;
    count++;
    printf("Produced: %d\n", item);
    print_buffer();

    pthread_cond_signal(&cond_consumer);
    pthread_mutex_unlock(&mutex);

    sleep(1);
  }
}

void *consumer(void *arg) {
  int item;
  while (1) {
    pthread_mutex_lock(&mutex);

    while (count == 0) {
      printf("Consumer waiting — buffer empty\n");
      pthread_cond_wait(&cond_consumer, &mutex);
    }

    item = buffer[count - 1];
    count--;
    printf("Consumed: %d\n", item);
    print_buffer();

    pthread_cond_signal(&cond_producer);
    pthread_mutex_unlock(&mutex);

    sleep(2); // consumer is slower than producer now
  }
}

int main() {
  pthread_t prod, cons;

  initialize_sync_objects();

  pthread_create(&prod, NULL, producer, NULL);
  pthread_create(&cons, NULL, consumer, NULL);

  pthread_join(prod, NULL);
  pthread_join(cons, NULL);

  cleanup_sync_objects();

  return 0;
}
