/*
 * producer_buffer_4_items_explained.c
 *
 * Same code as producer_buffer_4_items.c, but with detailed notes.
 *
 * Big idea:
 *   - One producer thread creates random numbers.
 *   - One consumer thread removes numbers.
 *   - They share a buffer with 4 slots.
 *   - The producer sleeps 1 second, consumer sleeps 2 seconds, so the
 *     producer is faster and the buffer will often fill up.
 *
 * This version uses:
 *   - mutex: protects shared variables buffer[] and count
 *   - cond_producer: producer waits here when buffer is full
 *   - cond_consumer: consumer waits here when buffer is empty
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* The shared buffer can hold 4 integers at a time. */
#define BUFFER_SIZE 4

/* Shared data.
 *
 * buffer[] stores the produced numbers.
 * count tells how many valid items are currently in buffer[].
 *
 * This code stores new items at buffer[count] and removes from
 * buffer[count - 1], so it behaves like a stack: last in, first out.
 */
int buffer[BUFFER_SIZE];
int count = 0;

/* Synchronization objects.
 *
 * mutex protects buffer[] and count.
 * cond_producer is where the producer sleeps when there is no room.
 * cond_consumer is where the consumer sleeps when there is no item.
 *
 * These are declared first, then initialized in initialize_sync_objects().
 */
pthread_mutex_t mutex;
pthread_cond_t cond_producer;
pthread_cond_t cond_consumer;

/* Initialize the mutex and condition variables before creating threads.
 *
 * NULL means "use default attributes."
 * If any init fails, perror prints the reason and exit stops the program.
 */
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

/* Destroy the synchronization objects after the threads are done.
 *
 * In this program the threads loop forever, so pthread_join never returns
 * unless the loops are changed later. Still, this is the correct cleanup
 * pattern for pthread objects that were initialized with pthread_*_init.
 */
void cleanup_sync_objects() {
  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond_producer);
  pthread_cond_destroy(&cond_consumer);
}

/* Print the current buffer contents.
 *
 * Slots before count are considered full.
 * Slots at count or after are considered empty and printed as "_".
 *
 * Example:
 *   [42|17|_|_] count=2
 * means two items are in the buffer.
 */
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

/* Producer thread function.
 *
 * pthread_create requires this exact style:
 *   void *function_name(void *arg)
 *
 * The arg parameter is not used here because main passes NULL.
 */
void *producer(void *arg) {
  int item;
  while (1) {
    /* Make the item outside the lock because rand() does not need to
     * touch the shared buffer or shared count. */
    item = rand() % 100;

    /* Enter the critical section.
     * From here until pthread_mutex_unlock, only one thread can touch
     * buffer[] and count. */
    pthread_mutex_lock(&mutex);

    /* If the buffer is full, the producer cannot add another item.
     *
     * Use while, not if:
     *   - pthread_cond_wait can wake up even if nobody signaled
     *   - another thread could change the condition before we run again
     *
     * pthread_cond_wait does this atomically:
     *   1. unlock mutex
     *   2. sleep on cond_producer
     *   3. wake up later and re-lock mutex before returning
     */
    while (count == BUFFER_SIZE) {
      printf("Producer waiting — buffer full\n");
      pthread_cond_wait(&cond_producer, &mutex);
    }

    /* There is room now, so insert at the next open slot. */
    buffer[count] = item;
    count++;
    printf("Produced: %d\n", item);
    print_buffer();

    /* Wake the consumer because the buffer definitely has at least one
     * item now. */
    pthread_cond_signal(&cond_consumer);

    /* Leave the critical section so the consumer can use the buffer. */
    pthread_mutex_unlock(&mutex);

    /* Producer waits 1 second before making another item. */
    sleep(1);
  }
}

/* Consumer thread function.
 *
 * This is the mirror image of producer:
 *   - wait if empty
 *   - remove one item
 *   - signal producer that a slot is free
 */
void *consumer(void *arg) {
  int item;
  while (1) {
    /* Lock before reading count or touching buffer[]. */
    pthread_mutex_lock(&mutex);

    /* If the buffer is empty, there is nothing to consume.
     *
     * The consumer sleeps on cond_consumer until the producer signals
     * that a new item was added.
     */
    while (count == 0) {
      printf("Consumer waiting — buffer empty\n");
      pthread_cond_wait(&cond_consumer, &mutex);
    }

    /* Remove the most recently produced item.
     *
     * Since count is the number of full slots, the last full slot is
     * buffer[count - 1].
     */
    item = buffer[count - 1];
    count--;
    printf("Consumed: %d\n", item);
    print_buffer();

    /* Wake the producer because one slot is now free. */
    pthread_cond_signal(&cond_producer);

    /* Unlock so the producer can add more items. */
    pthread_mutex_unlock(&mutex);

    /* Consumer sleeps 2 seconds, so it is slower than the producer.
     * This makes the buffer fill up and shows the producer waiting. */
    sleep(2); // consumer is slower than producer now
  }
}

/* main creates the two threads and waits for them.
 *
 * Since producer and consumer both loop forever, the joins never finish
 * in normal use. Stop the program with Ctrl-C.
 */
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
