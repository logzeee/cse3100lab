/* =========================================================================
 * cond-div-1.c — annotated for Lecture 17
 *
 * THIS FILE IS THE BROKEN VERSION.  It is identical to cond-div.c except
 * for ONE LINE in the producer:
 *
 *     cond-div.c   :   pthread_cond_broadcast(&pdata->ready_cond);
 *     cond-div-1.c :   pthread_cond_signal(&pdata->ready_cond);    <-- bug
 *
 * The lecture's own header even warns:
 *
 *     "Note do not be surprised if this code does not work"
 *
 * READ THIS WHOLE COMMENT BEFORE THE CODE
 * ---------------------------------------
 *
 * Why does swapping broadcast for signal break things?
 *
 *   pthread_cond_signal  -> wakes ONE waiter (an unspecified one).
 *   pthread_cond_broadcast -> wakes EVERY waiter.
 *
 * In cond-div there are TWO consumers waiting on the SAME condition var
 * (`ready_cond`), and BOTH of them legitimately need to be woken up
 * whenever new data arrives, because we don't know in advance which
 * consumer the data is "for".
 *
 * THE BUG SCENARIO (try input: 7)
 *
 *   1. Producer puts data=7, ready=1, signal(ready_cond).
 *   2. ONE consumer wakes -- say consumer 1 with p=2.
 *      It checks 7 % 2 != 0, sets its bit in `checked` (= 0b010 = 2),
 *      does NOT clear `ready`, does NOT signal anyone, unlocks.
 *      Loops back -> sees ready==1 AND its bit is set -> sleeps again.
 *   3. Consumer 2 (p=3) is STILL sleeping from before.  Nobody ever
 *      told it that data is ready.  It misses the chance to check.
 *   4. Producer is now blocked: it tries to place a new number, but
 *      `ready` is still 1, so it sleeps on `processed_cond`.
 *   5. DEADLOCK.  No one is going to wake anybody.
 *
 * THE LESSON
 * ----------
 *   Use BROADCAST when more than one waiter on the same condition
 *   variable could legitimately need to act on the event.
 *
 *   Use SIGNAL only when at most ONE waiter could proceed (e.g., the
 *   producer waiting on processed_cond — there's only one producer).
 *
 * Compile:  gcc -Wall cond-div-1.c -o cond-div-1 -lpthread
 * Run:      ./cond-div-1   (try entering 7 and watch it hang)
 * ========================================================================= */

// Note do not be surprised if this code does not work

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_CONSUMERS 2
#define FLAG 6                  //== bitmap meaning "consumers 1 AND 2 looked"

#define SET_BIT(v, i)   (v) |= (1 << (i))
#define CHECK_BIT(v, i) (v) &  (1 << (i))

/* Same shared-data struct as cond-div.c — see that file's comments for
 * the full breakdown of every field. */
typedef struct {
  int data;
  int ready;
  int checked;
  pthread_mutex_t mutex;
  pthread_cond_t ready_cond;
  pthread_cond_t processed_cond;
} data_t;

typedef struct {
  int id;
  int p;
  data_t *pdata;
} thread_arg_t;

/* =========================================================================
 * consumer  (UNCHANGED from cond-div.c)
 *
 *   Listens for ready_cond.  When awoken, decides whether the current
 *   data is divisible by its prime `p`.  If yes, consumes and clears
 *   ready.  If no, sets its bit in `checked`.
 *
 *   With BROADCAST, both consumers always get a fair chance.
 *   With SIGNAL  (the bug below), only one wakes per producer event,
 *                so the other one can stay asleep forever.
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
    int d = -1;

    // lines beween lock and unlock try to get data
    pthread_mutex_lock(&pdata->mutex);

    // wait if data is not ready, or it is ready but I already checked
    while (!pdata->ready || CHECK_BIT(pdata->checked, my_id)) {
      printf("consumer thread %d(p=%d) waiting for data from producer...\n",
             my_id, p);
      pthread_cond_wait(&pdata->ready_cond, &pdata->mutex);
    }

    if (pdata->data < 0) {
      done = 1;
    } else {
      if ((pdata->data % p) == 0) {
        // it's for me. grab it.
        printf("consumer thread %d(p=%d): use data=%d\n", my_id, p,
               pdata->data);
        d = pdata->data;
        pdata->ready = 0;
        pthread_cond_signal(&pdata->processed_cond);
      } else {
        printf("consumer thread %d(p=%d): checked data=%d. Not for me.\n",
               my_id, p, pdata->data);
        SET_BIT(pdata->checked, my_id);
        // one can check if all consumers have checked.
        if (pdata->checked == FLAG) {
          //== When the producer uses signal(), we may NEVER reach this
          //== branch — because only ONE consumer was woken, only ONE
          //== bit can ever get set per data.  So `checked` becomes 2
          //== or 4 but never 6.  That's the deadlock door.
          printf("Everyone checked.\n");
          d = -1;
          // d = pdata->data;
          pdata->ready = 0;
          pthread_cond_signal(&pdata->processed_cond);
        }
      }
    }
    pthread_mutex_unlock(&pdata->mutex);
    // add something to process the data
    if (d >= 0) {
      printf("consumer thread %d(p=%d): done with processing data %d\n", my_id,
             p, d);
    }
  }
  printf("consumer thread %d(p=%d) exiting\n", my_id, p);
  pthread_exit(NULL);
}

/* =========================================================================
 * producer  (BUG IS HERE)
 *
 *   The structure is identical to cond-div.c — same wait-on-processed,
 *   same write-data-and-mark-ready.  The single different line is at the
 *   bottom: pthread_cond_signal instead of pthread_cond_broadcast.
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
      v = -1;
    }

    pthread_mutex_lock(&pdata->mutex);
    while (pdata->ready) {
      printf("producer: waiting for %d to be processed.\n", pdata->data);
      pthread_cond_wait(&pdata->processed_cond, &pdata->mutex);
    }
    pdata->data = v;
    pdata->ready = 1;
    pdata->checked = 0;
    printf("producer placed data %d\n", v);
    // if signal, only one consumer will check
    // pthread_cond_signal(&pdata->ready_cond);
    // if we change pthread_cond_broadcast() to pthread_cond_signal()
    // what will happen?
    // pthread_cond_broadcast(&pdata->ready_cond);
    pthread_cond_signal(&pdata->ready_cond);
    //== ^^^ THE BUG:  signal() wakes only ONE consumer.
    //== If that one happens to find data % p != 0, it goes back to sleep
    //== and the OTHER consumer never even sees this data.  Then `ready`
    //== stays 1 forever, the producer blocks on processed_cond, and the
    //== whole program deadlocks.
    pthread_mutex_unlock(&pdata->mutex);
    done = (v < 0);
  }

  printf("producer exiting...\n");
  pthread_exit(NULL);
}

/* =========================================================================
 * main  (UNCHANGED from cond-div.c)
 *
 *   Spawns 1 producer and NUM_CONSUMERS consumers, each with a prime,
 *   then joins.  See cond-div_explained.c for full commentary.
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

  /* Initialize mutex and condition variable objects */
  pthread_mutex_init(&data.mutex, NULL);
  pthread_cond_init(&data.ready_cond, NULL);
  pthread_cond_init(&data.processed_cond, NULL);

  data.ready = 0; // no data is ready yet

  // create a producer
  thread_args[0].id = 0;
  thread_args[0].p = 0;
  thread_args[0].pdata = &data;
  rv = pthread_create(&threads[0], NULL, producer, &thread_args[0]);
  assert(rv == 0);

  // create consumers
  for (i = 1; i <= NUM_CONSUMERS; i++) {
    // prepare arguments
    thread_args[i].id = i;
    thread_args[i].p = primes[i - 1];
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
 * EXAM TAKEAWAY
 *
 *   Q: When should I use signal() vs broadcast() with condition variables?
 *
 *   A: Ask yourself: "How many waiters could legitimately make progress
 *      after the state change I just performed?"
 *
 *        - Exactly one (e.g., a single producer waiting for room) -> signal
 *        - Possibly more (e.g., several consumers each with their own
 *          predicate to re-evaluate, OR you bulk-changed the state) ->
 *          broadcast
 *
 *      When in doubt: broadcast is always SAFE (just wastes some CPU
 *      waking up threads that immediately go back to sleep).  signal is
 *      faster but DANGEROUS when multiple waiters share a cond var.
 *
 *      That single principle is what separates cond-div.c (works) from
 *      cond-div-1.c (deadlocks).
 * ========================================================================= */
