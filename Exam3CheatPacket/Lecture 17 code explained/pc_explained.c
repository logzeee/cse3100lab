/* =========================================================================
 * pc.c — annotated for Lecture 17
 *
 * The classic Producer / Consumer with a SINGLE-SLOT shared buffer.
 *
 * READING THIS FILE
 *   The original code is preserved exactly.  Comments starting with `//==`
 *   or living in /* ... block comments ... */ /* are the new, beginner-friendly
 *   notes I added.  Everything else is the original Lecture 17 code.
 *
 * THE BIG IDEA
 *   - The PRODUCER thread makes random numbers and PUTS them into a 1-slot
 *     buffer.
 *   - The CONSUMER thread TAKES those numbers out and "uses" them (just
 *     prints, in this demo).
 *
 *   They run AT THE SAME TIME on different threads, so without protection
 *   they could step on each other.  Two kinds of trouble:
 *
 *     1. Both touching `count` and `buffer[]` at once -> data race.
 *        FIX: a mutex (pthread_mutex_t).  Only one thread holds it at a time.
 *
 *     2. Producer wants to put when buffer is FULL,
 *        or consumer wants to take when buffer is EMPTY.
 *        FIX: condition variables (pthread_cond_t).  A thread can SLEEP
 *        until the other thread changes the world and wakes it up.
 *
 * EVERY classical producer/consumer solution = mutex + condition vars.
 *
 * Compile:  gcc -Wall pc.c -o pc -lpthread
 * Run:      ./pc        (loops forever; Ctrl-C to stop)
 * ========================================================================= */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE 1 // Size of the buffer
                      //== With size 1 the buffer is either EMPTY (count==0)
                      //== or FULL (count==1).  Easiest case to reason about.

int buffer[BUFFER_SIZE];
int count = 0; // Number of items in the buffer
               //== `count` is the SHARED STATE the two conditions key off:
               //==   producer waits when count == BUFFER_SIZE  (no room)
               //==   consumer waits when count == 0            (nothing to eat)

//== Three synchronization objects, all accessed by both threads:
pthread_mutex_t mutex;          //== one lock that protects buffer[] and count
pthread_cond_t  cond_producer;  //== producer sleeps here when buffer is full
pthread_cond_t  cond_consumer;  //== consumer sleeps here when buffer is empty

/* -------------------------------------------------------------------------
 * initialize_sync_objects()
 *   Plain setup function — just calls the *_init functions and bails out
 *   if any of them fail.  In real production code you'd want to clean up
 *   the things that DID succeed before exiting; for a teaching demo this
 *   is fine.
 * ------------------------------------------------------------------------- */
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
  //== Mirror of init: throw away the kernel resources backing the
  //== mutex / cond vars.  Forgetting this leaks memory in long-running
  //== programs.
  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond_producer);
  pthread_cond_destroy(&cond_consumer);
}

/* =========================================================================
 * producer thread
 *   Loop forever:
 *     1. Make an item (random number).
 *     2. Lock the mutex (so only WE touch the buffer).
 *     3. While the buffer is full, go to sleep on cond_producer.
 *        ** pthread_cond_wait magic: it ATOMICALLY unlocks the mutex,
 *           sleeps, and on wake-up RE-LOCKS the mutex before returning. **
 *     4. Put the item into the buffer, bump count.
 *     5. Signal cond_consumer in case the consumer was sleeping.
 *     6. Unlock the mutex.
 *     7. sleep(1) outside the lock (so we don't hog it while idle).
 * ========================================================================= */
void *producer(void *arg) {
  int item;
  while (1) {
    item = rand() % 100; // Produce a random item
                          //== rand() is a quick way to get test data.
                          //== (Note: rand() isn't thread-safe on every
                          //==  system, but for one producer it's fine.)
    pthread_mutex_lock(&mutex);

    // Wait until there's space in the buffer
    //== The `while` is mandatory, NEVER use `if` here.  Reasons:
    //==   a) Spurious wakeups: pthread_cond_wait may return without
    //==      anyone having signaled.  POSIX explicitly allows this.
    //==   b) Even if we WERE signaled, by the time we re-acquire the
    //==      mutex some other producer may have refilled the buffer.
    //== Either way -> recheck the predicate, only proceed if it's safe.
    while (count == BUFFER_SIZE) {
      pthread_cond_wait(&cond_producer, &mutex);
    }

    // Add item to buffer
    buffer[count] = item;       //== since count is 0 here, this is buffer[0]
    count++;                    //== now buffer is "full" (count == 1)
    printf("Produced: %d\n", item);

    // Signal the consumer that there's a new item
    //== signal() wakes up ONE waiter on cond_consumer.  If nobody is
    //== waiting, signal() is a no-op (NOT remembered for later).
    //== That's OK here because the consumer's `while` loop will see
    //== count > 0 the next time it gets the lock anyway.
    pthread_cond_signal(&cond_consumer);
    pthread_mutex_unlock(&mutex);

    sleep(1); // Sleep for a while to simulate production time
              //== sleep happens OUTSIDE the lock.  If we slept while
              //== holding the mutex, the consumer couldn't even check
              //== the buffer for a whole second.  Always release locks
              //== as soon as you're done with the shared state.
  }
}

/* =========================================================================
 * consumer thread
 *   Mirror of producer:
 *     1. Lock the mutex.
 *     2. While the buffer is empty, sleep on cond_consumer.
 *     3. Take an item out, decrement count.
 *     4. Signal cond_producer in case the producer was waiting for room.
 *     5. Unlock the mutex.
 *
 *   Note: this consumer does NOT sleep after consuming.  That makes it
 *   a "fast eater" — it'll usually be sitting in pthread_cond_wait,
 *   waking up the instant the producer puts something in.
 * ========================================================================= */
void *consumer(void *arg) {
  int item;
  while (1) {
    pthread_mutex_lock(&mutex);

    // Wait until there's an item in the buffer
    while (count == 0) {
            printf("consumer enters while loop\n");
                                //== This printf is a teaching aid; it shows
                                //== you the consumer is about to call
                                //== cond_wait.  Notice the LOCK is still
                                //== held when this printf runs (cond_wait
                                //== will release it during the sleep).
      pthread_cond_wait(&cond_consumer, &mutex);
            printf("signal\n");
                                //== Printed when cond_wait returns.
                                //== Either we got signaled OR a spurious
                                //== wakeup happened.  We loop back and
                                //== recheck `count == 0`.
    }

    // Remove item from buffer
    item = buffer[count - 1];   //== with count==1, that's buffer[0]
    count--;                    //== now count==0, buffer is "empty" again
    printf("Consumed: %d\n", item);

    // Signal the producer that there's space in the buffer
    pthread_cond_signal(&cond_producer);
    pthread_mutex_unlock(&mutex);

    /* sleep(1); // Sleep for a while to simulate consumption time */
                                //== Commented out so the consumer is
                                //== "always hungry."  Try uncommenting it
                                //== to see how the rhythm changes.
  }
}

/* =========================================================================
 * main:
 *   1. Initialize the mutex and the two condition variables.
 *   2. Spawn the producer and consumer threads.
 *   3. pthread_join blocks until each thread exits.
 *      In THIS program the threads loop forever, so the joins never
 *      return — `cleanup_sync_objects()` is reached only if you change
 *      the threads to actually exit (e.g., a fixed loop count).
 * ========================================================================= */
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

/* =========================================================================
 * EXAM-READY SUMMARY OF THE PATTERN
 *
 *   producer:                           consumer:
 *     LOCK(m);                            LOCK(m);
 *     while (FULL)                        while (EMPTY)
 *         WAIT(cv_p, m);                      WAIT(cv_c, m);
 *     put item;                           take item;
 *     SIGNAL(cv_c);                       SIGNAL(cv_p);
 *     UNLOCK(m);                          UNLOCK(m);
 *
 *   Memorize this skeleton — it covers 90% of producer/consumer questions.
 *   The other 10% adds: multiple producers/consumers, larger buffer,
 *   barriers for warm-up, or a "broadcast" instead of "signal" when more
 *   than one waiter could proceed (see cond-div.c next).
 * ========================================================================= */
