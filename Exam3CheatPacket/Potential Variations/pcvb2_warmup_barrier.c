/* =====================================================================
 * pcvb2_warmup_barrier.c
 *
 * BARRIER VARIATION 2: "WARM-UP" barrier — the same trick HW8/food.c uses.
 *
 * Problem this fixes:
 *   You have producers AND consumers, and the consumers also do some
 *   one-time setup (e.g., put their orders into a shared queue) BEFORE
 *   the producers should start working.  If a producer starts too early,
 *   it sees an empty queue and exits without doing anything.
 *
 * Fix:
 *   A barrier with trip count == NUM_PRODUCERS + NUM_CONSUMERS.
 *     - Each consumer does its setup, THEN waits at the barrier.
 *     - Each producer immediately waits at the barrier (does nothing first).
 *   Result: no producer can start until every consumer has finished
 *   setup, because the barrier won't open until everyone has arrived.
 *
 * Then the rest is a normal bounded-buffer producer/consumer:
 *   producers cook items, push them into a circular buffer; consumers
 *   pop one item each.  Items here are integers 1..N pulled off a
 *   shared "work queue" (just an array index for simplicity).
 *
 * Compile:  gcc -Wall pcvb2_warmup_barrier.c -o pcvb2 -lpthread
 * ===================================================================== */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_PRODUCERS 2
#define NUM_CONSUMERS 4         // also = number of "orders" placed
#define BUFFER_SIZE   3

// ----- Phase 1: the "order queue" the consumers prefill ----------------
// Each consumer puts ONE order on this queue during setup.  Producers
// will pop orders, "cook" them, and put the cooked item in the buffer.
static int  orders[NUM_CONSUMERS];
static int  num_orders = 0;             // grows during setup
static int  next_order = 0;             // producers' read index
static pthread_mutex_t orders_mtx = PTHREAD_MUTEX_INITIALIZER;

// ----- Phase 2: the bounded buffer producers fill, consumers drain -----
static int buffer[BUFFER_SIZE];
static int in = 0, out = 0, count = 0;
static pthread_mutex_t buf_mtx     = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  can_produce = PTHREAD_COND_INITIALIZER;
static pthread_cond_t  can_consume = PTHREAD_COND_INITIALIZER;

static pthread_barrier_t warmup;

void *consumer(void *arg) {
    int id = *(int *)arg;

    // === SETUP: place one order on the shared queue ===================
    pthread_mutex_lock(&orders_mtx);
    orders[num_orders++] = id * 10;     // any id-derived value
    pthread_mutex_unlock(&orders_mtx);

    // === Wait for ALL threads to finish setup =========================
    pthread_barrier_wait(&warmup);

    // === Real work: take one cooked item from the buffer ==============
    pthread_mutex_lock(&buf_mtx);
    while (count == 0) {
        pthread_cond_wait(&can_consume, &buf_mtx);
    }
    int item = buffer[out];
    out = (out + 1) % BUFFER_SIZE;
    count--;
    pthread_cond_signal(&can_produce);
    pthread_mutex_unlock(&buf_mtx);

    printf("    [C%d] received cooked item %d\n", id, item);
    return NULL;
}

void *producer(void *arg) {
    int id = *(int *)arg;

    // Producers do NOTHING in setup; they just wait their turn.
    pthread_barrier_wait(&warmup);

    while (1) {
        // Pull one order from the queue.  If none left, this producer is done.
        int my_order = -1;
        pthread_mutex_lock(&orders_mtx);
        if (next_order < num_orders) {
            my_order = orders[next_order++];
        }
        pthread_mutex_unlock(&orders_mtx);

        if (my_order < 0) break;

        usleep(100000);                  // "cooking"
        int cooked = my_order + 1;

        pthread_mutex_lock(&buf_mtx);
        while (count == BUFFER_SIZE) {
            pthread_cond_wait(&can_produce, &buf_mtx);
        }
        buffer[in] = cooked;
        in = (in + 1) % BUFFER_SIZE;
        count++;
        printf("[P%d] cooked %d (count=%d)\n", id, cooked, count);
        pthread_cond_signal(&can_consume);
        pthread_mutex_unlock(&buf_mtx);
    }
    return NULL;
}

int main(void) {
    pthread_t producers[NUM_PRODUCERS];
    pthread_t consumers[NUM_CONSUMERS];
    int       pids[NUM_PRODUCERS], cids[NUM_CONSUMERS];

    // Trip count includes EVERY thread.  This is exactly the food.c trick.
    pthread_barrier_init(&warmup, NULL, NUM_PRODUCERS + NUM_CONSUMERS);

    for (int i = 0; i < NUM_CONSUMERS; i++) {
        cids[i] = i + 1;
        pthread_create(&consumers[i], NULL, consumer, &cids[i]);
    }
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        pids[i] = i + 1;
        pthread_create(&producers[i], NULL, producer, &pids[i]);
    }

    for (int i = 0; i < NUM_CONSUMERS; i++) pthread_join(consumers[i], NULL);
    for (int i = 0; i < NUM_PRODUCERS; i++) pthread_join(producers[i], NULL);

    pthread_barrier_destroy(&warmup);
    pthread_mutex_destroy(&orders_mtx);
    pthread_mutex_destroy(&buf_mtx);
    pthread_cond_destroy(&can_produce);
    pthread_cond_destroy(&can_consume);
    return 0;
}
