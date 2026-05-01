/* pc_circular.c
 * 1 producer, 1 consumer, CIRCULAR buffer of size SIZE.
 * Only difference vs pc_basic.c: in/out indices and `count` instead
 * of a single boolean.
 *
 * gcc -Wall pc_circular.c -o pc_circular -lpthread
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define SIZE    4
#define N_ITEMS 10

int buf[SIZE];
int in = 0, out = 0, count = 0;

pthread_mutex_t m;
pthread_cond_t  cP;
pthread_cond_t  cC;

/* pthread_create requires every thread function to take a void* argument
 * and return a void*. We don't pass anything to this thread, so the
 * parameter "arg" is unused. (void)arg; silences the unused-parameter
 * warning under -Wall.
 */
void *producer(void *arg) {
    (void)arg;
    for (int i = 1; i <= N_ITEMS; i++) {
        pthread_mutex_lock(&m);
        while (count == SIZE)                     // FULL
            pthread_cond_wait(&cP, &m);

        buf[in] = i;
        in = (in + 1) % SIZE;                     // wrap around
        count++;
        printf("produced %d (count=%d)\n", i, count);

        pthread_cond_signal(&cC);
        pthread_mutex_unlock(&m);
    }
    return 0;
}

void *consumer(void *arg) {
    (void)arg;
    for (int i = 1; i <= N_ITEMS; i++) {
        pthread_mutex_lock(&m);
        while (count == 0)                        // EMPTY
            pthread_cond_wait(&cC, &m);

        int got = buf[out];
        out = (out + 1) % SIZE;                   // wrap around
        count--;
        printf("              consumed %d (count=%d)\n", got, count);

        pthread_cond_signal(&cP);
        pthread_mutex_unlock(&m);
    }
    return 0;
}

int main(void) {
    pthread_mutex_init(&m,  NULL);
    pthread_cond_init(&cP, NULL);
    pthread_cond_init(&cC, NULL);

    pthread_t p, c;
    pthread_create(&p, 0, producer, 0);
    pthread_create(&c, 0, consumer, 0);
    pthread_join(p, 0);
    pthread_join(c, 0);

    pthread_mutex_destroy(&m);
    pthread_cond_destroy(&cP);
    pthread_cond_destroy(&cC);
    return 0;
}
