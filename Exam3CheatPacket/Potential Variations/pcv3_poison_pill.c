/* =====================================================================
 * pcv3_poison_pill.c
 *
 * VARIATION 3: Clean shutdown using a "POISON PILL" sentinel value.
 *
 * Problem this solves:
 *   In pcv2 we needed an extra shared counter (`items_left`) and the
 *   consumer loop had to check TWO conditions to know it was done.
 *   That logic is easy to get wrong on an exam.
 *
 * Cleaner pattern:
 *   - When the producer is finished making real items, it pushes ONE
 *     "poison pill" item (here: -1) for EACH consumer.
 *   - Each consumer consumes items normally; when it sees the poison
 *     pill, it exits.  No extra counters, no extra conditions.
 *
 * Important detail:
 *   - With N consumers we need N pills, otherwise some consumer would
 *     wait forever for an item that never comes.
 *
 * Compile:  gcc -Wall pcv3_poison_pill.c -o pcv3 -lpthread
 * ===================================================================== */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE   4
#define NUM_CONSUMERS 3
#define NUM_REAL_ITEMS 10
#define POISON_PILL  (-1)   // sentinel: "you're done, go home"

static int buffer[BUFFER_SIZE];
static int in = 0, out = 0, count = 0;

static pthread_mutex_t mutex       = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  can_produce = PTHREAD_COND_INITIALIZER;
static pthread_cond_t  can_consume = PTHREAD_COND_INITIALIZER;

// Helper used by both real items AND the poison pills.
static void push(int item) {
    pthread_mutex_lock(&mutex);
    while (count == BUFFER_SIZE) {
        pthread_cond_wait(&can_produce, &mutex);
    }
    buffer[in] = item;
    in = (in + 1) % BUFFER_SIZE;
    count++;
    pthread_cond_signal(&can_consume);
    pthread_mutex_unlock(&mutex);
}

void *producer(void *arg) {
    (void)arg;
    for (int i = 1; i <= NUM_REAL_ITEMS; i++) {
        push(i);
        printf("Produced %d\n", i);
        usleep(80000);
    }
    // One pill per consumer so EVERY consumer eventually gets one.
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        push(POISON_PILL);
    }
    printf("Producer: sent %d real items + %d poison pills\n",
           NUM_REAL_ITEMS, NUM_CONSUMERS);
    return NULL;
}

void *consumer(void *arg) {
    int id = *(int *)arg;
    while (1) {
        pthread_mutex_lock(&mutex);
        while (count == 0) {
            pthread_cond_wait(&can_consume, &mutex);
        }
        int item = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        count--;
        pthread_cond_signal(&can_produce);
        pthread_mutex_unlock(&mutex);

        if (item == POISON_PILL) {
            printf("    [C%d] got poison pill -> exiting\n", id);
            break;
        }
        printf("    [C%d] consumed %d\n", id, item);
        usleep(120000);
    }
    return NULL;
}

int main(void) {
    pthread_t prod;
    pthread_t consumers[NUM_CONSUMERS];
    int       cids[NUM_CONSUMERS];

    pthread_create(&prod, NULL, producer, NULL);
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        cids[i] = i + 1;
        pthread_create(&consumers[i], NULL, consumer, &cids[i]);
    }

    pthread_join(prod, NULL);
    for (int i = 0; i < NUM_CONSUMERS; i++) pthread_join(consumers[i], NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&can_produce);
    pthread_cond_destroy(&can_consume);
    return 0;
}
