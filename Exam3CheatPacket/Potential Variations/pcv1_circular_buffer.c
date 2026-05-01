/* =====================================================================
 * pcv1_circular_buffer.c
 *
 * VARIATION 1: Single producer, single consumer, CIRCULAR (ring) buffer.
 *
 * What's new vs. the simplest "buffer of size 1" version:
 *   - The buffer holds N items, not just 1.
 *   - We track THREE numbers:
 *       in    -> index where the producer will write next
 *       out   -> index the consumer will read next
 *       count -> how many items are currently sitting in the buffer
 *   - "in" and "out" wrap around with modulo (% BUFFER_SIZE).
 *
 * Why all three?
 *   If we only had `in` and `out`, when in == out we couldn't tell if the
 *   buffer is EMPTY or FULL — both look the same.  `count` removes that
 *   ambiguity.  (The other classic trick is to leave one slot unused, but
 *   keeping a `count` is clearer for learning.)
 *
 * Compile:  gcc -Wall pcv1_circular_buffer.c -o pcv1 -lpthread
 * ===================================================================== */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE 4       // ring capacity
#define NUM_ITEMS  12       // how many items the producer makes total

static int buffer[BUFFER_SIZE];
static int in = 0;          // next write slot
static int out = 0;         // next read slot
static int count = 0;       // current number of items

static pthread_mutex_t mutex       = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  can_produce = PTHREAD_COND_INITIALIZER;  // "not full"
static pthread_cond_t  can_consume = PTHREAD_COND_INITIALIZER;  // "not empty"

void *producer(void *arg) {
    (void)arg;
    for (int i = 1; i <= NUM_ITEMS; i++) {
        pthread_mutex_lock(&mutex);

        // WAIT IN A LOOP — never `if`. After we wake up, the buffer may
        // already be full again because many things could have happened
        // (in this 1-prod / 1-cons demo only spurious wakeups, but in
        // general other producers could refill the buffer first).
        while (count == BUFFER_SIZE) {
            pthread_cond_wait(&can_produce, &mutex);
        }

        buffer[in] = i;
        in = (in + 1) % BUFFER_SIZE;   // wrap around
        count++;
        printf("Produced %2d  (count=%d, in=%d, out=%d)\n", i, count, in, out);

        // Tell ANY consumer waiting on "not empty" that there's something now.
        pthread_cond_signal(&can_consume);
        pthread_mutex_unlock(&mutex);

        usleep(100000);  // simulate work
    }
    return NULL;
}

void *consumer(void *arg) {
    (void)arg;
    for (int i = 1; i <= NUM_ITEMS; i++) {
        pthread_mutex_lock(&mutex);

        while (count == 0) {
            pthread_cond_wait(&can_consume, &mutex);
        }

        int item = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        count--;
        printf("Consumed %2d (count=%d, in=%d, out=%d)\n", item, count, in, out);

        pthread_cond_signal(&can_produce);
        pthread_mutex_unlock(&mutex);

        usleep(150000); // consumer is slightly slower -> buffer will sometimes fill
    }
    return NULL;
}

int main(void) {
    pthread_t prod, cons;
    pthread_create(&prod, NULL, producer, NULL);
    pthread_create(&cons, NULL, consumer, NULL);
    pthread_join(prod, NULL);
    pthread_join(cons, NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&can_produce);
    pthread_cond_destroy(&can_consume);
    return 0;
}
