/*
 * producer_buffer_4_barrier_explained.c
 *
 * Same code as producer_buffer_4_barrier.c, but with detailed notes.
 *
 * Big idea:
 *   - Producer and consumer threads are created.
 *   - Both print that they are ready.
 *   - Both wait at a barrier.
 *   - Once both have arrived, the barrier releases them together.
 *   - Then they run the producer/consumer loop with a buffer of 4 slots.
 *
 * Why use a barrier here?
 *   A barrier is a meeting point. It blocks every thread that calls
 *   pthread_barrier_wait until N threads have arrived. Then all N are
 *   released at the same time. We use it so the producer and consumer
 *   are guaranteed to start at the same instant, instead of one running
 *   for a while before the other even exists.
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* The shared buffer can hold 4 integers at a time. */
#define BUFFER_SIZE 4

/* The barrier needs to know how many threads will meet at it. We have
 * one producer and one consumer, so 2. */
#define NUM_THREADS 2  // producer + consumer

/* Shared data.
 *
 * buffer[] stores the produced numbers.
 * count tells how many valid items are currently in buffer[].
 *
 * Items are inserted at buffer[count] and removed from buffer[count - 1],
 * so the buffer behaves like a stack: last in, first out.
 */
int buffer[BUFFER_SIZE];
int count = 0;

/* Synchronization objects.
 *
 * mutex          : protects buffer[] and count.
 * cond_producer  : producer sleeps here when there is no room.
 * cond_consumer  : consumer sleeps here when there is no item.
 * barrier        : both threads wait here once at startup so they
 *                  begin the main loop together.
 */
pthread_mutex_t mutex;
pthread_cond_t cond_producer;
pthread_cond_t cond_consumer;
pthread_barrier_t barrier;  // <--- new

/* Initialize the mutex, condition variables, and barrier before creating
 * the threads.
 *
 * NULL means "use default attributes."
 *
 * The third argument to pthread_barrier_init is the number of threads
 * that must call pthread_barrier_wait before any of them are released.
 * Here that number is NUM_THREADS == 2.
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
  // barrier waits for NUM_THREADS threads before releasing all
  if (pthread_barrier_init(&barrier, NULL, NUM_THREADS) != 0) {
    perror("Barrier initialization failed");
    exit(EXIT_FAILURE);
  }
}

/* Destroy every object that was initialized above.
 *
 * In this program the threads loop forever, so pthread_join never
 * returns and this function is never reached during a normal run.
 * It is still the correct cleanup pattern, and the exam expects it.
 */
void cleanup_sync_objects() {
  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond_producer);
  pthread_cond_destroy(&cond_consumer);
  pthread_barrier_destroy(&barrier);  // <--- new
}

/* Print the current buffer contents.
 *
 * Slots before count are considered full.
 * Slots at count or after are considered empty and printed as "_".
 *
 * Example output:
 *   [42|17|_|_] count=2
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
 * Step 1: print that we are at the barrier.
 * Step 2: pthread_barrier_wait blocks until the consumer also arrives.
 * Step 3: print that we are starting (this only runs after the barrier).
 * Step 4: enter the normal produce loop.
 */
void *producer(void *arg) {
  printf("Producer ready and waiting at barrier...\n");
  pthread_barrier_wait(&barrier);  // wait for consumer to be ready too
  printf("Producer starting!\n");

  int item;
  while (1) {
    /* Make the item outside the lock because rand() does not touch
     * the shared buffer or count. */
    item = rand() % 100;

    /* Enter the critical section. From here until pthread_mutex_unlock,
     * only one thread can read or write buffer[] and count. */
    pthread_mutex_lock(&mutex);

    /* If the buffer is full, the producer cannot insert. Sleep on
     * cond_producer until a consumer makes room and signals us.
     *
     * Use while, not if:
     *   - cond_wait can wake up for no reason (spurious wakeup).
     *   - Even after waking, another thread could have already
     *     re-filled the buffer. We must re-check.
     */
    while (count == BUFFER_SIZE) {
      printf("Producer waiting — buffer full\n");
      pthread_cond_wait(&cond_producer, &mutex);
    }

    /* There is room now. Insert at the next free slot. */
    buffer[count] = item;
    count++;
    printf("Produced: %d\n", item);
    print_buffer();

    /* Wake the consumer because there is at least one item now. */
    pthread_cond_signal(&cond_consumer);

    /* Leave the critical section so the consumer can take the item. */
    pthread_mutex_unlock(&mutex);

    /* Producer is the faster thread (1 second). Sleep is OUTSIDE the
     * lock so we do not block the consumer while we wait. */
    sleep(1);
  }
}

/* Consumer thread function.
 *
 * Mirror image of the producer:
 *   - print, wait at the barrier, print again
 *   - then loop: lock, wait if empty, take item, signal, unlock, sleep
 */
void *consumer(void *arg) {
  printf("Consumer ready and waiting at barrier...\n");
  pthread_barrier_wait(&barrier);  // wait for producer to be ready too
  printf("Consumer starting!\n");

  int item;
  while (1) {
    /* Lock before reading count or buffer[]. */
    pthread_mutex_lock(&mutex);

    /* If the buffer is empty, sleep on cond_consumer until the producer
     * inserts something and signals us. */
    while (count == 0) {
      printf("Consumer waiting — buffer empty\n");
      pthread_cond_wait(&cond_consumer, &mutex);
    }

    /* Remove the most recently produced item.
     * count is the number of full slots, so the last full slot is
     * buffer[count - 1]. */
    item = buffer[count - 1];
    count--;
    printf("Consumed: %d\n", item);
    print_buffer();

    /* Wake the producer because one slot is now free. */
    pthread_cond_signal(&cond_producer);

    /* Unlock so the producer can add more. */
    pthread_mutex_unlock(&mutex);

    /* Consumer is the slower thread (2 seconds). This makes the
     * buffer fill up so we can see the producer wait when full. */
    sleep(2);
  }
}

/* main creates the two threads and waits for them.
 *
 * The threads loop forever, so pthread_join never returns in normal
 * use. Stop the program with Ctrl-C.
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
