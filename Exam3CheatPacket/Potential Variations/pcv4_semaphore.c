/* =====================================================================
 * pcv4_semaphore.c
 *
 * VARIATION 4: Producer / consumer using SEMAPHORES instead of
 *              condition variables.
 *
 * The textbook semaphore-based bounded-buffer uses three semaphores:
 *
 *     sem_t empty   -> counts EMPTY slots in the buffer.  Initialized
 *                      to BUFFER_SIZE.  A producer does sem_wait(&empty)
 *                      before adding (i.e., "claim a free slot").
 *
 *     sem_t full    -> counts FULL slots (i.e., items waiting to be
 *                      consumed).  Initialized to 0.  A consumer does
 *                      sem_wait(&full) before removing.
 *
 *     sem_t mutex   -> binary semaphore (acts like a lock) protecting
 *                      the buffer indices.  Initialized to 1.
 *
 * Why this is elegant:
 *   We don't need any `while (...) cond_wait(...)` loops because
 *   semaphores DO the counting and the blocking for us.
 *
 * Note for macOS users:
 *   POSIX `sem_init` for unnamed semaphores is DEPRECATED on macOS — it
 *   compiles but always returns -1.  This file uses `sem_open` (named
 *   semaphores), which works on both macOS and Linux.  On Linux you can
 *   alternatively use sem_init.
 *
 * Compile:  gcc -Wall pcv4_semaphore.c -o pcv4 -lpthread
 * ===================================================================== */

#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>          // O_CREAT
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE 4
#define NUM_ITEMS  10

static int buffer[BUFFER_SIZE];
static int in = 0, out = 0;

static sem_t *empty;        // counts free slots
static sem_t *full;         // counts items
static sem_t *mtx;          // protects in/out (binary semaphore == lock)

void *producer(void *arg) {
    (void)arg;
    for (int i = 1; i <= NUM_ITEMS; i++) {
        sem_wait(empty);                // claim a free slot (block if none)
        sem_wait(mtx);                  // enter critical section
        buffer[in] = i;
        in = (in + 1) % BUFFER_SIZE;
        printf("Produced %d\n", i);
        sem_post(mtx);                  // leave critical section
        sem_post(full);                 // announce one new item
        usleep(100000);
    }
    return NULL;
}

void *consumer(void *arg) {
    (void)arg;
    for (int i = 1; i <= NUM_ITEMS; i++) {
        sem_wait(full);                 // wait for an item
        sem_wait(mtx);
        int item = buffer[out];
        out = (out + 1) % BUFFER_SIZE;
        printf("    Consumed %d\n", item);
        sem_post(mtx);
        sem_post(empty);                // a slot just became free
        usleep(150000);
    }
    return NULL;
}

int main(void) {
    // Use unique names; clean up any leftovers from a previous run.
    sem_unlink("/pcv4_empty");
    sem_unlink("/pcv4_full");
    sem_unlink("/pcv4_mtx");

    empty = sem_open("/pcv4_empty", O_CREAT, 0644, BUFFER_SIZE);
    full  = sem_open("/pcv4_full",  O_CREAT, 0644, 0);
    mtx   = sem_open("/pcv4_mtx",   O_CREAT, 0644, 1);
    if (empty == SEM_FAILED || full == SEM_FAILED || mtx == SEM_FAILED) {
        perror("sem_open"); return 1;
    }

    pthread_t prod, cons;
    pthread_create(&prod, NULL, producer, NULL);
    pthread_create(&cons, NULL, consumer, NULL);
    pthread_join(prod, NULL);
    pthread_join(cons, NULL);

    sem_close(empty); sem_close(full); sem_close(mtx);
    sem_unlink("/pcv4_empty"); sem_unlink("/pcv4_full"); sem_unlink("/pcv4_mtx");
    return 0;
}
