/* =====================================================================
 * pc_barrier_explained.c
 *
 *   MULTI-producer / MULTI-consumer with a CIRCULAR buffer
 *   PLUS a hand-built BARRIER so all threads start at the same instant.
 *
 * This file is the "level 2" companion to pc_explained.c. Read that
 * one first; it explains the basic mutex / condition-variable pattern.
 *
 * What you will learn here:
 *   1. How to scale producer/consumer up to MANY producers and consumers.
 *   2. How to use a CIRCULAR (ring) buffer with `in`, `out`, `count`.
 *   3. How to terminate cleanly (no infinite loop, no Ctrl-C needed).
 *   4. How a BARRIER works, and how to BUILD ONE from scratch using
 *      only a mutex + a condition variable. (pthread_barrier_t exists
 *      on Linux but is NOT in macOS' libpthread, so writing your own
 *      is a common exam question.)
 *
 * Compile:   gcc -Wall -O2 pc_barrier_explained.c -o pc_barrier -lpthread
 * Run:       ./pc_barrier
 *
 * =====================================================================
 *  PART 0  --  WHAT IS A BARRIER?
 * =====================================================================
 *
 *  A barrier is a synchronization point that says:
 *
 *      "No thread proceeds past this line until ALL N threads have
 *       arrived at this line."
 *
 *  Picture a starting line in a race: even if you arrive early, you
 *  must wait for the slowest runner before the gun fires.
 *
 *  Typical uses:
 *    - Make sure all worker threads start a phase together.
 *    - Make sure phase N finishes everywhere before phase N+1 begins
 *      (e.g. parallel matrix updates between iterations).
 *
 *  How to BUILD one from scratch:
 *    Keep a counter `arrived`. Each thread increments it under a mutex.
 *      - If you are the LAST one (arrived == total), you reset the
 *        counter and BROADCAST to wake everybody.
 *      - Otherwise, you WAIT on a condition variable until that
 *        broadcast happens.
 *
 *  The tricky bit: you need a `generation` counter so that a barrier
 *  can be reused safely. Without it, fast threads racing into the
 *  NEXT barrier would mess up the count of slow threads still leaving
 *  the previous one. We'll see exactly how to use it below.
 *
 * =====================================================================
 *  PART 1  --  TERMINATION FOR PRODUCER/CONSUMER
 * =====================================================================
 *
 *  Infinite-loop producer/consumer (like pc.c) is fine for a demo, but
 *  on an exam you usually want to PROVE that everything ended cleanly.
 *
 *  Rule of thumb:
 *    * Producers count down a finite number of items, then announce
 *      "I'm done" by incrementing `producers_done` and broadcasting on
 *      `can_consume` so any sleeping consumer wakes up.
 *    * Consumers loop until two conditions BOTH hold:
 *          buffer is empty   AND   all producers have finished
 *      ...because there will be no more arrivals to wait for.
 *
 *  Read the consumer's wait loop carefully -- this pattern is exactly
 *  what graders look for.
 * ===================================================================== */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* ---- problem-size knobs ---- */
#define BUFFER_SIZE          10
#define NUM_PRODUCERS        2
#define NUM_CONSUMERS        2
#define ITEMS_PER_PRODUCER   8
#define TOTAL_ITEMS          (NUM_PRODUCERS * ITEMS_PER_PRODUCER)

/* =====================================================================
 *  PART 2  --  HAND-BUILT BARRIER
 *
 *  Fields:
 *    mutex      -> protects the other fields
 *    cond       -> threads wait here until the last one arrives
 *    arrived    -> how many threads have hit the barrier in this round
 *    total      -> how many we expect (set at init time)
 *    generation -> increments every time the barrier "fires".
 *                  Each waiting thread captures the current generation
 *                  before sleeping; it wakes up only once the barrier
 *                  has advanced. This makes the barrier reusable.
 * ===================================================================== */
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    int arrived;
    int total;
    int generation;
} barrier_t;

void barrier_init(barrier_t *b, int total) {
    pthread_mutex_init(&b->mutex, NULL);
    pthread_cond_init(&b->cond, NULL);
    b->arrived    = 0;
    b->total      = total;
    b->generation = 0;
}

void barrier_destroy(barrier_t *b) {
    pthread_mutex_destroy(&b->mutex);
    pthread_cond_destroy(&b->cond);
}

/* The CORE function. Walk through it slowly. */
void barrier_wait(barrier_t *b) {
    pthread_mutex_lock(&b->mutex);

    /* Step 1. Remember which "round" we are part of.
     *         If we end up sleeping, we'll wake only when the barrier
     *         advances past this generation. */
    int my_generation = b->generation;

    /* Step 2. Mark ourselves as having arrived. */
    b->arrived++;

    if (b->arrived == b->total) {
        /* Step 3a. We are the LAST to arrive. Open the gate:
         *           - reset the counter for the next round
         *           - bump the generation
         *           - wake everyone who is waiting
         *         Note: we do NOT wait ourselves; we just walk through. */
        b->arrived = 0;
        b->generation++;
        pthread_cond_broadcast(&b->cond);
    } else {
        /* Step 3b. Not the last one yet. Sleep until the generation
         *           changes. Loop guards against spurious wakeups
         *           (the same `while` rule as in pc_explained.c). */
        while (my_generation == b->generation) {
            pthread_cond_wait(&b->cond, &b->mutex);
        }
    }

    pthread_mutex_unlock(&b->mutex);
}

/* =====================================================================
 *  PART 3  --  SHARED STATE FOR THE PRODUCER/CONSUMER
 *
 *  Circular buffer:
 *      `in`  is where the next PRODUCED item will be written.
 *      `out` is where the next CONSUMED item will be read.
 *      `count` is how many items are currently sitting in the buffer.
 *
 *  After each insert/remove, advance the index with modulo:
 *      in  = (in  + 1) % BUFFER_SIZE;
 *      out = (out + 1) % BUFFER_SIZE;
 *
 *  This lets us reuse the array forever -- the indices wrap around.
 * ===================================================================== */
int buffer[BUFFER_SIZE];
int count          = 0;
int in             = 0;
int out            = 0;
int consumed_total = 0;   /* for stats / sanity check at the end       */
int producers_done = 0;   /* how many producers have already finished */

pthread_mutex_t mutex;
pthread_cond_t  can_produce;                              /* wake producers */
pthread_cond_t  can_consume;                              /* wake consumers */
barrier_t       start_barrier;                            /* "ready, set, go" */

/* =====================================================================
 *  PART 4  --  PRODUCER
 *
 *  Each producer:
 *    (1) Waits at the START barrier so all threads launch together.
 *    (2) Produces ITEMS_PER_PRODUCER items, sleeping briefly between
 *        each one (just to make output interleave nicely).
 *    (3) After its last item, increments `producers_done` and
 *        BROADCASTS on can_consume. Why broadcast?  Because there may
 *        be several consumers blocked on `count == 0`, and now they
 *        all need to wake up to notice "no more producers will come"
 *        and exit cleanly.
 * ===================================================================== */
void *producer(void *arg) {
    int id = *(int *)arg;

    /* (1) Synchronize the start. */
    barrier_wait(&start_barrier);

    /* (2) Produce items. */
    for (int i = 1; i <= ITEMS_PER_PRODUCER; i++) {
        int item = id * 100 + i;     /* a tag so we can see which producer made it */

        pthread_mutex_lock(&mutex);

        /* Wait while the buffer is FULL. */
        while (count == BUFFER_SIZE) {
            pthread_cond_wait(&can_produce, &mutex);
        }

        /* Insert into the ring buffer at position `in`. */
        buffer[in] = item;
        in = (in + 1) % BUFFER_SIZE;
        count++;
        printf("Producer %d produced %d (count = %d)\n", id, item, count);

        /* Wake one consumer (signal is fine when only consumers wait
         * on can_consume and they all do the same thing). */
        pthread_cond_signal(&can_consume);
        pthread_mutex_unlock(&mutex);

        usleep(200000);   /* 200 ms; outside the lock! */
    }

    /* (3) Mark this producer as done and wake EVERYONE so consumers
     *     can re-check their termination condition. */
    pthread_mutex_lock(&mutex);
    producers_done++;
    pthread_cond_broadcast(&can_consume);
    pthread_mutex_unlock(&mutex);

    return NULL;
}

/* =====================================================================
 *  PART 5  --  CONSUMER
 *
 *  Termination logic is the interesting part:
 *
 *     while ( count == 0  AND  producers_done < NUM_PRODUCERS )
 *         wait
 *
 *  This means: "keep sleeping ONLY IF there is nothing to eat AND more
 *  food is still coming." The loop exits as soon as either:
 *     (a) there's something to consume, OR
 *     (b) all producers are done (so no more food will ever appear).
 *
 *  After the loop, we re-check:
 *     if (count == 0 && producers_done == NUM_PRODUCERS) -> truly done
 *  Otherwise (count > 0) we consume one item.
 * ===================================================================== */
void *consumer(void *arg) {
    int id = *(int *)arg;

    /* Hold at the start barrier. */
    barrier_wait(&start_barrier);

    while (1) {
        pthread_mutex_lock(&mutex);

        /* Sleep while empty AND more producers may still arrive. */
        while (count == 0 && producers_done < NUM_PRODUCERS) {
            pthread_cond_wait(&can_consume, &mutex);
        }

        /* Termination check: empty buffer AND no producers left. */
        if (count == 0 && producers_done == NUM_PRODUCERS) {
            pthread_mutex_unlock(&mutex);   /* don't forget to unlock! */
            break;
        }

        /* Otherwise count > 0; safe to take from the ring buffer. */
        int item = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        count--;
        consumed_total++;
        printf("Consumer %d consumed %d (total = %d)\n",
               id, item, consumed_total);

        pthread_cond_signal(&can_produce);
        pthread_mutex_unlock(&mutex);

        usleep(300000);   /* 300 ms simulated work, outside the lock */
    }

    return NULL;
}

/* =====================================================================
 *  PART 6  --  MAIN
 *
 *  - Initialize the barrier with TOTAL = NUM_PRODUCERS + NUM_CONSUMERS
 *    so that ALL workers have to arrive before anybody runs.
 *  - Pass each thread its own id via a unique int address. (NEVER pass
 *    the address of a loop variable that gets overwritten between
 *    iterations -- a classic exam gotcha that causes all threads to
 *    see the same id.)
 *  - Join everyone, then clean up.
 * ===================================================================== */
int main(void) {
    pthread_t producers[NUM_PRODUCERS];
    pthread_t consumers[NUM_CONSUMERS];
    int producer_ids[NUM_PRODUCERS];
    int consumer_ids[NUM_CONSUMERS];

    pthread_mutex_init(&mutex,       NULL);
    pthread_cond_init(&can_produce,  NULL);
    pthread_cond_init(&can_consume,  NULL);
    barrier_init(&start_barrier, NUM_PRODUCERS + NUM_CONSUMERS);

    for (int i = 0; i < NUM_PRODUCERS; i++) {
        producer_ids[i] = i + 1;
        pthread_create(&producers[i], NULL, producer, &producer_ids[i]);
    }
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        consumer_ids[i] = i + 1;
        pthread_create(&consumers[i], NULL, consumer, &consumer_ids[i]);
    }

    for (int i = 0; i < NUM_PRODUCERS; i++)  pthread_join(producers[i], NULL);
    for (int i = 0; i < NUM_CONSUMERS; i++)  pthread_join(consumers[i], NULL);

    printf("All done. Consumed %d item(s) (expected %d).\n",
           consumed_total, TOTAL_ITEMS);

    barrier_destroy(&start_barrier);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&can_produce);
    pthread_cond_destroy(&can_consume);
    return 0;
}

/* =====================================================================
 *  PART 7  --  EXAM-STYLE Q & A (BARRIER + MULTI-P/C)
 * =====================================================================
 *
 *  Q1. Why do you need a `generation` field in the barrier?
 *  A1. Without it, the barrier can only be used ONCE. With several
 *      rounds, fast threads might race ahead and start incrementing
 *      `arrived` for round 2 before slow threads have left round 1,
 *      stealing their wake-up. Capturing `my_generation` before
 *      sleeping, and looping until `b->generation` changes, ensures
 *      a thread only wakes for ITS round.
 *
 *  Q2. Why broadcast() in barrier_wait, not signal()?
 *  A2. When the last thread arrives, EVERY waiter must wake. signal()
 *      would wake just one of them and the rest would sleep forever.
 *
 *  Q3. Why is the start barrier set to NUM_PRODUCERS + NUM_CONSUMERS?
 *  A3. We want both groups (producers AND consumers) to be ready
 *      before either starts. If the barrier total were just NUM_
 *      PRODUCERS, consumers might start consuming before producers
 *      were ready, or vice versa.
 *
 *  Q4. Why broadcast on can_consume when a producer finishes, instead
 *      of signal?
 *  A4. Multiple consumers may be sleeping with `count == 0`. They all
 *      need to recheck their termination condition. signal() would
 *      wake only one and the others could hang forever.
 *
 *  Q5. Why pass &producer_ids[i] to pthread_create instead of &i?
 *  A5. The loop variable `i` is overwritten on every iteration. By
 *      the time a thread reads it, it might already be the next
 *      iteration's value. Each thread needs its OWN stable storage.
 *
 *  Q6. What if a consumer is sleeping and the producers all finish
 *      before that consumer wakes?
 *  A6. The last producer broadcasts on `can_consume`. The sleeping
 *      consumer wakes, the loop predicate (count==0 AND producers_done
 *      <NUM_PRODUCERS) is now false, so it exits the wait, hits the
 *      "truly done" check, unlocks, and returns. No deadlock.
 *
 *  Q7. Could two consumers consume the SAME item?
 *  A7. No. Reading `out` and incrementing it is done under the mutex,
 *      so only one consumer at a time can advance `out`. The next
 *      consumer reads the NEW value.
 *
 *  Q8. Where is the critical section in `barrier_wait`?
 *  A8. Everything between `pthread_mutex_lock(&b->mutex)` and
 *      `pthread_mutex_unlock(&b->mutex)`. The cond_wait inside the
 *      loop temporarily releases the mutex while sleeping, then
 *      re-acquires it before returning -- you do not need to (and
 *      MUST not) unlock manually around it.
 *
 *  Q9. Could you use sem_wait/sem_post (semaphores) instead?
 *  A9. Yes. A counting semaphore initialized to BUFFER_SIZE for
 *      "empty slots" and another initialized to 0 for "filled slots"
 *      gives the textbook semaphore-based producer/consumer. The
 *      mutex+condvar pattern shown here is more flexible (you can
 *      wait on richer predicates) and is what your reference files
 *      use, so it's what graders will expect.
 *
 *  Q10. What's the FIFO guarantee of this circular buffer?
 *  A10. Items leave in the order they entered, because every producer
 *       advances `in` under the lock and every consumer advances
 *       `out` under the lock. With multiple producers, the order of
 *       insertion among the producers themselves is non-deterministic
 *       (whoever grabs the lock first), but once items are in, FIFO
 *       on the way out is preserved.
 *
 * ===================================================================== */
