/* =====================================================================
 * pcv2_multi_pc.c
 *
 * VARIATION 2: MULTIPLE producers and MULTIPLE consumers.
 *
 * Key teaching points:
 *   1. Same circular buffer as variation 1, BUT now many producers and
 *      consumers race for the same slots, so the `while` loop around
 *      pthread_cond_wait becomes essential — between "wakeup" and
 *      "re-acquiring the mutex", another producer/consumer may have
 *      already changed the state.
 *
 *   2. signal vs broadcast:
 *        pthread_cond_signal    -> wakes up ONE waiter
 *        pthread_cond_broadcast -> wakes up ALL waiters
 *      With multiple producers and consumers, `signal` is usually fine
 *      and faster (only one slot was added/removed, so only one waiter
 *      can actually proceed).  Use `broadcast` when MULTIPLE waiters
 *      could potentially proceed (e.g. you bulk-added several items, or
 *      different waiters wait on different sub-conditions).
 *
 *   3. Clean shutdown:
 *      The producers count down a SHARED `items_left`.  When that hits 0
 *      every producer exits.  Consumers stop when (items_left == 0) AND
 *      (count == 0).  This is one of the simplest clean-shutdown schemes;
 *      see pcv3_poison_pill.c for a different ("sentinel value") approach.
 *
 * Compile:  gcc -Wall pcv2_multi_pc.c -o pcv2 -lpthread
 * ===================================================================== */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE   5
#define NUM_PRODUCERS 3
#define NUM_CONSUMERS 4
#define TOTAL_ITEMS   30   // shared across all producers

static int buffer[BUFFER_SIZE];
static int in = 0, out = 0, count = 0;
static int items_left = TOTAL_ITEMS;     // shared remaining work
static int next_item  = 1;               // monotonically increasing item id

static pthread_mutex_t mutex       = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  can_produce = PTHREAD_COND_INITIALIZER;
static pthread_cond_t  can_consume = PTHREAD_COND_INITIALIZER;

void *producer(void *arg) {
    int id = *(int *)arg;
    while (1) {
        pthread_mutex_lock(&mutex);

        // Either buffer is full or we're out of work to claim.
        while (items_left > 0 && count == BUFFER_SIZE) {
            pthread_cond_wait(&can_produce, &mutex);
        }
        if (items_left == 0) {
            // Wake any consumer that is waiting on an empty buffer so it
            // can also notice "we're done".
            pthread_cond_broadcast(&can_consume);
            pthread_mutex_unlock(&mutex);
            break;
        }

        int item = next_item++;
        items_left--;
        buffer[in] = item;
        in = (in + 1) % BUFFER_SIZE;
        count++;
        printf("[P%d] produced %2d  (count=%d, items_left=%d)\n",
               id, item, count, items_left);

        pthread_cond_signal(&can_consume);   // one new item -> wake one consumer
        pthread_mutex_unlock(&mutex);

        usleep(80000);
    }
    return NULL;
}

void *consumer(void *arg) {
    int id = *(int *)arg;
    while (1) {
        pthread_mutex_lock(&mutex);

        // Wait while buffer is empty AND there's still work coming.
        while (count == 0 && items_left > 0) {
            pthread_cond_wait(&can_consume, &mutex);
        }
        // If both the buffer AND the producer queue are empty, we're done.
        if (count == 0 && items_left == 0) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        int item = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        count--;
        printf("            [C%d] consumed %2d (count=%d)\n", id, item, count);

        pthread_cond_signal(&can_produce);
        pthread_mutex_unlock(&mutex);

        usleep(120000);
    }
    return NULL;
}

int main(void) {
    pthread_t producers[NUM_PRODUCERS];
    pthread_t consumers[NUM_CONSUMERS];
    int       pids[NUM_PRODUCERS], cids[NUM_CONSUMERS];

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

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&can_produce);
    pthread_cond_destroy(&can_consume);
    return 0;
}
