/* =====================================================================
 * pcvb1_round_barrier.c
 *
 * BARRIER VARIATION 1: ROUND-BASED producer/consumer pipeline.
 *
 * Pattern:
 *   - There are NUM_ROUNDS "rounds".
 *   - In each round:
 *        Step A: Every producer pushes ONE item.
 *        Step B: Everybody waits at the barrier.
 *        Step C: Every consumer pops ONE item.
 *        Step D: Everybody waits at the barrier again.
 *
 * Why use a barrier here?
 *   The barrier guarantees the WHOLE GROUP advances together.  This is
 *   useful when each round has to be "complete" before the next one can
 *   start — e.g., simulation steps, image-processing passes, machine
 *   learning batches, ...
 *
 *   In this model:
 *     - The barrier between A and C makes sure ALL items for the round
 *       are in the buffer before any consumer starts taking them out.
 *     - The barrier at the end of the round makes sure no producer for
 *       round N+1 starts pushing until every consumer for round N has
 *       finished.
 *
 *   Result: no condition variables needed at all!  The barrier alone
 *   is enough to keep producers and consumers in lock-step.
 *
 * Note (macOS): pthread_barrier_t is not in the default macOS pthread
 * headers.  This file targets a Linux-style pthread implementation
 * (which is what the homework uses).
 *
 * Compile:  gcc -Wall pcvb1_round_barrier.c -o pcvb1 -lpthread
 * ===================================================================== */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_PRODUCERS 3
#define NUM_CONSUMERS 3
#define NUM_ROUNDS    4
// One slot per producer per round is enough because everyone waits at
// the barrier before consumers start popping.
#define BUFFER_SIZE   (NUM_PRODUCERS)

static int buffer[BUFFER_SIZE];
static int in = 0, out = 0;          // simple circular indices
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_barrier_t barrier;

void *producer(void *arg) {
    int id = *(int *)arg;
    for (int r = 0; r < NUM_ROUNDS; r++) {
        // Encode producer id and round into the item for clarity.
        int item = id * 100 + r;

        pthread_mutex_lock(&mutex);
        buffer[in] = item;
        in = (in + 1) % BUFFER_SIZE;
        pthread_mutex_unlock(&mutex);

        printf("[P%d] round %d pushed %d\n", id, r, item);

        // Step B: wait for everyone (all producers AND all consumers).
        pthread_barrier_wait(&barrier);

        // While consumers run step C, producers just wait at the next barrier.
        pthread_barrier_wait(&barrier);  // Step D
    }
    return NULL;
}

void *consumer(void *arg) {
    int id = *(int *)arg;
    for (int r = 0; r < NUM_ROUNDS; r++) {
        // Step B: wait until producers have filled the buffer for round r.
        pthread_barrier_wait(&barrier);

        pthread_mutex_lock(&mutex);
        int item = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        pthread_mutex_unlock(&mutex);

        printf("    [C%d] round %d popped  %d\n", id, r, item);

        // Step D: signal end-of-round.
        pthread_barrier_wait(&barrier);
    }
    return NULL;
}

int main(void) {
    pthread_t producers[NUM_PRODUCERS];
    pthread_t consumers[NUM_CONSUMERS];
    int       pids[NUM_PRODUCERS], cids[NUM_CONSUMERS];

    // Trip count = total threads sharing the rounds.
    pthread_barrier_init(&barrier, NULL, NUM_PRODUCERS + NUM_CONSUMERS);

    for (int i = 0; i < NUM_PRODUCERS; i++) {
        pids[i] = i + 1;
        pthread_create(&producers[i], NULL, producer, &pids[i]);
    }
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        cids[i] = i + 1;
        pthread_create(&consumers[i], NULL, consumer, &cids[i]);
    }

    for (int i = 0; i < NUM_PRODUCERS; i++) pthread_join(producers[i], NULL);
    for (int i = 0; i < NUM_CONSUMERS; i++) pthread_join(consumers[i], NULL);

    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&mutex);
    return 0;
}
