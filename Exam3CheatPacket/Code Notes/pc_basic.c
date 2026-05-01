/* pc_basic.c
 * 1 producer, 1 consumer, 1-slot buffer.
 * THE foundation pattern. Memorize this and the rest is decoration.
 *
 * gcc -Wall pc_basic.c -o pc_basic -lpthread
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#define N_ITEMS 5

int item;
int has_item = 0;

pthread_mutex_t m;
pthread_cond_t  cP;   // producer waits here
pthread_cond_t  cC;   // consumer waits here

/* SHORTCUT: if you don't need custom attributes, you can skip the
 * pthread_mutex_init / pthread_cond_init calls in main() AND the
 * matching pthread_mutex_destroy / pthread_cond_destroy calls by
 * using the static initializer macros at declaration time:
 *
 *     pthread_mutex_t m  = PTHREAD_MUTEX_INITIALIZER;
 *     pthread_cond_t  cP = PTHREAD_COND_INITIALIZER;
 *     pthread_cond_t  cC = PTHREAD_COND_INITIALIZER;
 *
 * Same effect as init(..., NULL), zero runtime cost, no destroy needed.
 * Only works for statically-allocated (global/static) variables with
 * default attributes.
 */

/* pthread_create requires every thread function to take a void* argument
 * and return a void*. We aren't passing anything to this thread, so we
 * just name the parameter "arg" and don't use it. The (void)arg; line
 * tells the compiler "I know it's unused, don't warn me about it."
 */
void *producer(void *arg) {
    (void)arg;
    for (int i = 1; i <= N_ITEMS; i++) {
        pthread_mutex_lock(&m);
        while (has_item)                          // FULL -> wait
            pthread_cond_wait(&cP, &m);

        item = i;
        has_item = 1;
        printf("produced %d\n", i);

        pthread_cond_signal(&cC);                 // wake consumer
        pthread_mutex_unlock(&m);
    }
    return 0;
}

void *consumer(void *arg) {
    (void)arg;
    for (int i = 1; i <= N_ITEMS; i++) {
        pthread_mutex_lock(&m);
        while (!has_item)                         // EMPTY -> wait
            pthread_cond_wait(&cC, &m);

        int got = item;
        has_item = 0;
        printf("              consumed %d\n", got);

        pthread_cond_signal(&cP);                 // wake producer
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
