/* =========================================================================
 * cond-div.c — annotated for Lecture 17
 *
 * NOT a normal producer/consumer.  This program is about ONE producer and
 * MANY consumers, where each consumer only takes the data IF the data is
 * divisible by THAT consumer's prime number.  It teaches:
 *
 *     - Why we sometimes need pthread_cond_BROADCAST (not signal).
 *     - How to use a SECOND condition variable so the producer can wait
 *       until the consumers are done with the current data.
 *     - How a BITMAP can track "which consumers have already looked at
 *       this datum" without needing a per-consumer flag.
 *
 *
 * THE STORY
 * ---------
 *   - Producer reads numbers from stdin and "places" them one at a time.
 *   - 2 consumers exist:
 *         consumer 1 with prime p = 2  -> wants data divisible by 2
 *         consumer 2 with prime p = 3  -> wants data divisible by 3
 *
 *   For each number the producer puts down:
 *     a) Producer signals everyone "data is ready".
 *     b) Each consumer wakes up, checks `data % p`:
 *          - if it's "for me" (divisible) -> consume it, mark not-ready.
 *          - if not -> set my bit in `checked` saying "I looked, skip me",
 *                     then go back to sleep.
 *     c) Once every consumer has checked (or one consumed), data is no
 *        longer "ready" and the producer is woken to place the next one.
 *
 *   Producer enters a NEGATIVE number (or EOF) to shut down the program.
 *
 *
 * IMPORTANT CONCEPT: BITMAP
 * -------------------------
 *   `pdata->checked` is an int used as a bitmap.
 *
 *     SET_BIT(v, i)   -> v |= (1 << i)        // turn bit i ON
 *     CHECK_BIT(v, i) -> v &  (1 << i)        // is bit i ON?  (truthy/0)
 *
 *   With NUM_CONSUMERS == 2 and consumer ids 1 and 2:
 *     - When consumer 1 marks itself,  bit 1 turns on  -> 0b010 = 2.
 *     - When consumer 2 marks itself,  bit 2 turns on  -> 0b100 = 4.
 *     - When BOTH have marked,         bits 1+2        -> 0b110 = 6.
 *
 *   That's why FLAG == 6 means "everyone has checked".
 *
 *
 * IMPORTANT CONCEPT: WHY BROADCAST?
 * ---------------------------------
 *   Producer puts a new datum and calls broadcast.  Both consumers wake
 *   up, look at it, and decide.  If we used signal() instead, only ONE
 *   consumer would wake up — and if that one's prime doesn't divide the
 *   data, the OTHER consumer never gets a chance to check it.  That bug
 *   is exactly what cond-div-1.c demonstrates.
 *
 * Compile:  gcc -Wall cond-div.c -o cond-div -lpthread
 * Run:      ./cond-div    (then type integers, end with a negative one)
 * ========================================================================= */

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_CONSUMERS 2
#define FLAG 6                  //== bitmap value meaning "consumers 1 AND 2
                                //== have both checked the current data"
                                //== (bit 1 + bit 2 == 0b010 + 0b100 == 6)

#define SET_BIT(v, i)   (v) |= (1 << (i))
#define CHECK_BIT(v, i) (v) &  (1 << (i))

/* -------------------------------------------------------------------------
 * data_t — the SHARED slot the producer writes and consumers read.
 *
 *   data           -> the integer the producer most recently wrote.
 *   ready          -> 1 if the data is fresh and waiting to be consumed.
 *   checked        -> bitmap; bit i set = consumer with id i already
 *                     looked at this datum and decided "not for me".
 *   mutex          -> protects every field above.
 *   ready_cond     -> consumers wait here for data to become ready.
 *   processed_cond -> producer waits here for consumers to be done with
 *                     the current data (i.e., ready becomes 0 again).
 * ------------------------------------------------------------------------- */
typedef struct {
  int data;
  int ready;
  int checked;
  pthread_mutex_t mutex;
  pthread_cond_t ready_cond;
  pthread_cond_t processed_cond;
} data_t;

/* What each thread receives in its `void *` argument */
typedef struct {
  int id;             //== consumer's index (1 or 2 here); 0 for producer
  int p;              //== consumer's prime (2 or 3); 0 for producer
  data_t *pdata;      //== pointer to the SAME shared data_t for everyone
} thread_arg_t;

/* =========================================================================
 * consumer
 *
 *   Loop:
 *     1. Lock the mutex.
 *     2. Wait until data is READY and *I* haven't already checked it.
 *        Two reasons we might wait:
 *           - data isn't ready yet (producer hasn't placed it)
 *           - data IS ready but I've checked it and said "not for me"
 *             so I'd just loop forever — instead I sleep until
 *             SOMEONE clears `ready` (someone consumed or all checked).
 *     3. If data < 0  -> the producer is signaling shutdown, exit loop.
 *        Else if data % p == 0:
 *           grab it, mark ready=0, signal processed_cond (wake producer).
 *        Else:
 *           set MY bit in `checked`.  If everyone has checked (== FLAG),
 *           also clear ready and wake the producer.
 *     4. Unlock the mutex.
 *     5. (OUTSIDE the lock) "process" the data if I really got one.
 * ========================================================================= */
void *consumer(void *t) {
  thread_arg_t *arg = t;

  // get parameters
  int my_id = arg->id;
  int p = arg->p;
  data_t *pdata = arg->pdata;

  int done = 0;

  printf("consumer thread %d(p=%d) started\n", my_id, p);

  while (!done) {
    int d = -1;                 //== local: holds the "real" data I consumed
                                //==        so I can process it after unlock.
                                //== -1 means "I didn't actually take any".

    // lines beween lock and unlock try to get data
    pthread_mutex_lock(&pdata->mutex);

    // wait if data is not ready, or it is ready but I already checked
    //== This is the trickiest predicate of the whole file:
    //==   keep sleeping while  (no data)  OR  (already looked at it).
    while (!pdata->ready || CHECK_BIT(pdata->checked, my_id)) {
      printf("consumer thread %d(p=%d) waiting for data from producer...\n",
             my_id, p);
      pthread_cond_wait(&pdata->ready_cond, &pdata->mutex);
    }

    if (pdata->data < 0) {
      //== Producer placed a negative number => shutdown.
      //== Note: ready is still 1 here.  This particular code path
      //== never clears it, but since `done = 1` we won't loop again.
      done = 1;
    } else {
      if ((pdata->data % p) == 0) {
        // it's for me. grab it.
        printf("consumer thread %d(p=%d): use data=%d\n", my_id, p,
               pdata->data);
        d = pdata->data;
        pdata->ready = 0;       //== clear "ready" so producer can move on
        pthread_cond_signal(&pdata->processed_cond);
                                //== one signal wakes the producer; nobody
                                //== else waits on processed_cond.
      } else {
        printf("consumer thread %d(p=%d): checked data=%d. Not for me.\n",
               my_id, p, pdata->data);
        SET_BIT(pdata->checked, my_id);
        // one can check if all consumers have checked.
        if (pdata->checked == FLAG) {
          //== Both consumer 1 (bit 1) and consumer 2 (bit 2) have looked.
          //== Nobody wanted the data -> drop it and let producer continue.
          printf("Everyone checked.\n");
          d = -1;
          pdata->ready = 0;
          pthread_cond_signal(&pdata->processed_cond);
        }
      }
    }
    pthread_mutex_unlock(&pdata->mutex);
    // add something to process the data
    //== "Processing" is done OUTSIDE the lock so the heavy work doesn't
    //== freeze the producer / other consumer.  In real code this might
    //== be a long computation, network call, etc.
    if (d >= 0) {
      printf("consumer thread %d(p=%d): done with processing data %d\n", my_id,
             p, d);
    }
  }
  printf("consumer thread %d(p=%d) exiting\n", my_id, p);
  pthread_exit(NULL);
}

/* =========================================================================
 * producer
 *
 *   Loop:
 *     1. Read an integer from the user (negative = shutdown).
 *     2. Lock the mutex.
 *     3. While `ready` is still 1 (the previous datum hasn't finished
 *        being consumed/checked yet), sleep on processed_cond.
 *     4. Write data, set ready = 1, clear `checked` bitmap.
 *     5. broadcast() on ready_cond -> wake EVERY consumer so each one
 *        gets a chance to check the new data.
 *     6. Unlock the mutex.
 *
 *   Why broadcast and not signal?  See the giant note at the top of the
 *   file.  Short answer: with signal we'd only wake ONE consumer; if that
 *   one says "not for me", the others never look and the data is silently
 *   lost.  cond-div-1.c is built on that bug to show what goes wrong.
 * ========================================================================= */
void *producer(void *t) {
  thread_arg_t *arg = t;

  // get parameters
  data_t *pdata = arg->pdata;

  int done = 0;

  while (!done) {
    printf("Producer: Enter an integer:\n");
    int v;

    if (scanf("%d", &v) != 1) {
      v = -1;                   //== EOF or junk input -> treat as shutdown
    }

    pthread_mutex_lock(&pdata->mutex);
    while (pdata->ready) {
      //== The previous data is still "ready" — i.e., hasn't been
      //== consumed AND not all consumers have checked it yet.
      //== Sleep until somebody clears it.
      printf("producer: waiting for %d to be processed.\n", pdata->data);
      pthread_cond_wait(&pdata->processed_cond, &pdata->mutex);
    }
    pdata->data = v;
    pdata->ready = 1;
    pdata->checked = 0;         //== reset bitmap so consumers can re-check
    printf("producer placed data %d\n", v);
    // if signal, only one consumer will check
    // pthread_cond_signal(&pdata->ready_cond);
    //== ^^ This commented-out line is the BUG version.  Compare with
    //== cond-div-1.c which actually uses signal() and breaks.
    pthread_cond_broadcast(&pdata->ready_cond);
    pthread_mutex_unlock(&pdata->mutex);
    done = (v < 0);
  }

  printf("producer exiting...\n");
  pthread_exit(NULL);
}

/* =========================================================================
 * main: setup, spawn threads, wait, clean up.
 *
 *   thread_args[0] is the producer (p == 0 here, just unused).
 *   thread_args[1..NUM_CONSUMERS] are consumers, each with a prime
 *     pulled from the `primes` array (consumer 1 -> 2, consumer 2 -> 3).
 *
 *   The arrays of pthread_t and thread_arg_t both hold all 3 threads.
 * ========================================================================= */
int main(int argc, char *argv[]) {
  static int primes[] = {2, 3, 5, 7, 11, 13, 0};
  int i, rv;

  data_t data;

  pthread_t threads[NUM_CONSUMERS + 1];
  thread_arg_t thread_args[NUM_CONSUMERS + 1];

  /* For portability, you can explicitly create threads in a joinable state */
  // define attr, initialize it, set the attribute, and destroy at the end
  // pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
                                //== Threads are joinable by default on Linux,
                                //== so we don't bother setting the attribute.

  /* Initialize mutex and condition variable objects */
  pthread_mutex_init(&data.mutex, NULL);
  pthread_cond_init(&data.ready_cond, NULL);
  pthread_cond_init(&data.processed_cond, NULL);

  data.ready = 0; // no data is ready yet
                  //== `data.data` and `data.checked` are not initialized
                  //== explicitly because consumers will only read them
                  //== AFTER ready becomes 1, at which point producer has
                  //== set both.

  // create a producer
  thread_args[0].id = 0;
  thread_args[0].p = 0;
  thread_args[0].pdata = &data;
  rv = pthread_create(&threads[0], NULL, producer, &thread_args[0]);
  assert(rv == 0);

  // create consumers
  for (i = 1; i <= NUM_CONSUMERS; i++) {
    // prepare arguments
    thread_args[i].id = i;                  //== id 1 then 2
    thread_args[i].p  = primes[i - 1];      //== p 2 then 3
    thread_args[i].pdata = &data;
    assert(thread_args[i].p != 0);

    rv = pthread_create(&threads[i], NULL, consumer, &thread_args[i]);
    assert(rv == 0);
  }

  /* Wait for all threads to complete */
  for (i = 0; i <= NUM_CONSUMERS; i++) {
    pthread_join(threads[i], NULL);
  }

  /* Clean up and exit */
  pthread_mutex_destroy(&data.mutex);
  pthread_cond_destroy(&data.ready_cond);
  pthread_cond_destroy(&data.processed_cond);
  return 0;
}

/* =========================================================================
 * MENTAL TRACE: producer types `6`
 *
 *   - producer: lock, ready==0 (no wait), data=6, ready=1, checked=0,
 *               broadcast(ready_cond), unlock.
 *   - both consumers wake.
 *
 *   - consumer 1 (p=2): lock, ready=1 and bit 1 not set -> proceeds.
 *                       6 % 2 == 0 -> "for me", consume.  ready=0.
 *                       signal(processed_cond). unlock. process(6).
 *
 *   - consumer 2 (p=3): lock.  Now ready==0, so the while predicate
 *                       (!ready || already_checked) is true again ->
 *                       sleeps on ready_cond, waiting for the next datum.
 *
 *   - producer: lock, ready==0 -> immediately reads next number. Done.
 *
 * MENTAL TRACE: producer types `7` (no consumer's prime divides 7)
 *
 *   - producer places 7, broadcasts.
 *   - consumer 1: 7 % 2 != 0 -> SET_BIT(checked, 1). checked = 0b010 = 2.
 *                  not equal to FLAG (6) -> just unlock and loop.
 *                  Back at top: ready==1 and bit 1 IS set -> sleep again.
 *   - consumer 2: 7 % 3 != 0 -> SET_BIT(checked, 2). checked = 0b110 = 6.
 *                  Equals FLAG -> ready=0, signal(processed_cond).
 *   - producer: wakes, ready==0, accepts next input.
 *
 * That second trace is exactly what cond-div-1.c gets WRONG when it uses
 * signal() instead of broadcast().
 * ========================================================================= */
