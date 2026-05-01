/* barrier_basic.c
 * Minimal pthread_barrier_t example.
 * N threads each do "phase 1", wait, then do "phase 2".
 * No barrier? Some threads start phase 2 while others still in phase 1.
 *
 * gcc -Wall barrier_basic.c -o barrier_basic -lpthread
 *
 * Note: macOS doesn't ship pthread_barrier_t. Tested on Linux/the
 * class server. The shim at the top makes it run on macOS too.
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#ifdef __APPLE__
typedef struct { pthread_mutex_t m; pthread_cond_t c;
                 int arrived, total, gen; } pthread_barrier_t;
typedef int pthread_barrierattr_t;
static int pthread_barrier_init(pthread_barrier_t *b,
        const pthread_barrierattr_t *a, unsigned n) { (void)a;
    pthread_mutex_init(&b->m, 0); pthread_cond_init(&b->c, 0);
    b->arrived = 0; b->total = (int)n; b->gen = 0; return 0; }
static int pthread_barrier_wait(pthread_barrier_t *b) {
    pthread_mutex_lock(&b->m); int g = b->gen;
    if (++b->arrived == b->total) { b->arrived = 0; b->gen++;
        pthread_cond_broadcast(&b->c); }
    else while (g == b->gen) pthread_cond_wait(&b->c, &b->m);
    pthread_mutex_unlock(&b->m); return 0; }
static int pthread_barrier_destroy(pthread_barrier_t *b) {
    pthread_mutex_destroy(&b->m); pthread_cond_destroy(&b->c); return 0; }
#endif

#define N 4

pthread_barrier_t bar;

void *worker(void *arg) {
    int id = *(int *)arg;

    printf("T%d phase 1\n", id);
    sleep(id);                                    // uneven work

    pthread_barrier_wait(&bar);                   // wait for everyone

    printf("T%d phase 2\n", id);                  // all start phase 2 together
    return 0;
}

int main(void) {
    pthread_t t[N];
    int id[N];

    pthread_barrier_init(&bar, NULL, N);          // trip count = N

    for (int i = 0; i < N; i++) { id[i] = i + 1;
        pthread_create(&t[i], 0, worker, &id[i]); }
    for (int i = 0; i < N; i++) pthread_join(t[i], 0);

    pthread_barrier_destroy(&bar);
    return 0;
}
