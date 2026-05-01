/* =====================================================================
 * pcv5_batch.c
 *
 * VARIATION 5: BATCH producer / consumer.
 *
 * Twist: every put/get moves K items at once instead of one item.
 *
 *   Producer  ->  pushes K items into the buffer atomically
 *   Consumer  ->  pops  K items out of the buffer atomically
 *
 * What changes vs. the single-item version:
 *
 *   Predicate the producer waits on:
 *     instead of  while (count == BUFFER_SIZE)        (no room for 1)
 *     we use      while (BUFFER_SIZE - count < BATCH) (no room for K)
 *
 *   Predicate the consumer waits on:
 *     instead of  while (count == 0)                  (need 1)
 *     we use      while (count < BATCH)               (need K)
 *
 * Why use cond_BROADCAST here?
 *   When a consumer removes K items, MULTIPLE producers may now have
 *   enough room to proceed (K >= 1).  Same idea on the consumer side.
 *   `signal` would still be CORRECT but might leave wake-able waiters
 *   sleeping unnecessarily, hurting throughput.
 *
 * Compile:  gcc -Wall pcv5_batch.c -o pcv5 -lpthread
 * ===================================================================== */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE   8
#define BATCH         3
#define NUM_BATCHES   5     // each producer does this many BATCH-sized pushes

static int buffer[BUFFER_SIZE];
static int in = 0, out = 0, count = 0;
static int next_item = 1;

static pthread_mutex_t mutex       = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  can_produce = PTHREAD_COND_INITIALIZER;
static pthread_cond_t  can_consume = PTHREAD_COND_INITIALIZER;

void *producer(void *arg) {
    int id = *(int *)arg;
    for (int b = 0; b < NUM_BATCHES; b++) {
        pthread_mutex_lock(&mutex);

        // Need ROOM FOR K items, not just 1.
        while (BUFFER_SIZE - count < BATCH) {
            pthread_cond_wait(&can_produce, &mutex);
        }

        printf("[P%d] producing batch:", id);
        for (int k = 0; k < BATCH; k++) {
            int item = next_item++;
            buffer[in] = item;
            in = (in + 1) % BUFFER_SIZE;
            count++;
            printf(" %d", item);
        }
        printf("   (count=%d)\n", count);

        // Many consumers may now be able to take K items -> broadcast.
        pthread_cond_broadcast(&can_consume);
        pthread_mutex_unlock(&mutex);
        usleep(80000);
    }
    return NULL;
}

void *consumer(void *arg) {
    int id = *(int *)arg;
    for (int b = 0; b < NUM_BATCHES; b++) {
        pthread_mutex_lock(&mutex);

        // Need K items available, not just 1.
        while (count < BATCH) {
            pthread_cond_wait(&can_consume, &mutex);
        }

        printf("    [C%d] consuming batch:", id);
        for (int k = 0; k < BATCH; k++) {
            int item = buffer[out];
            out = (out + 1) % BUFFER_SIZE;
            count--;
            printf(" %d", item);
        }
        printf("   (count=%d)\n", count);

        pthread_cond_broadcast(&can_produce);
        pthread_mutex_unlock(&mutex);
        usleep(120000);
    }
    return NULL;
}

int main(void) {
    pthread_t p1, c1;
    int pid = 1, cid = 1;
    pthread_create(&p1, NULL, producer, &pid);
    pthread_create(&c1, NULL, consumer, &cid);
    pthread_join(p1, NULL);
    pthread_join(c1, NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&can_produce);
    pthread_cond_destroy(&can_consume);
    return 0;
}
