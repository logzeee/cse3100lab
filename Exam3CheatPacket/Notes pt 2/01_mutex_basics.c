/* 01_mutex_basics.c
 * MUTEX = "Mutual Exclusion lock". The most basic shared-state protector.
 * Only ONE thread can hold the lock at a time.
 *
 * gcc -Wall 01_mutex_basics.c -o 01_mutex_basics -lpthread
 *
 * =============================================================================
 * WHY: WITHOUT A MUTEX, count++ IS A LIE.
 *
 *   count++ is really:   load count -> add 1 -> store count
 *   Two threads can interleave those steps and "lose" updates (data race).
 *
 * A mutex makes the whole load/add/store run as ONE indivisible block from
 * the program's point of view.
 *
 * =============================================================================
 * THE 4 THINGS YOU ALWAYS DO WITH A MUTEX
 *
 *   1. INIT        pthread_mutex_init(&m, NULL);     // OR static initializer
 *   2. LOCK        pthread_mutex_lock(&m);
 *   3. UNLOCK      pthread_mutex_unlock(&m);         // ALWAYS pair with lock
 *   4. DESTROY     pthread_mutex_destroy(&m);        // when fully done
 *
 * RULES OF THE ROAD
 *   - Every lock() must have exactly one unlock() on every code path.
 *     (Including error paths and early returns. Unlock before return.)
 *   - Hold the lock for the SHORTEST possible time. Never sleep / do I/O
 *     while holding a lock unless you absolutely must.
 *   - Don't lock the same mutex twice in the same thread (default mutex is
 *     non-recursive -> deadlock with itself).
 *   - When you need 2 locks, ALL threads must take them in the SAME ORDER,
 *     or you get a deadlock cycle.
 *
 * =============================================================================
 * TWO WAYS TO INITIALIZE
 *
 *   A) Static (global / file-scope), default attributes:
 *        pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
 *        // No matching destroy is *required* for the static initializer,
 *        // but calling it is harmless and good hygiene.
 *
 *   B) Dynamic (any storage), can pass attributes:
 *        pthread_mutex_t m;
 *        pthread_mutex_init(&m, NULL);
 *        ...
 *        pthread_mutex_destroy(&m);
 * =============================================================================
 */

#include <pthread.h>
#include <stdio.h>

#define N_THREADS 4
#define BUMPS     100000

long counter = 0;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;       // static init form

void *bumper(void *arg) {
    (void)arg;
    for (int i = 0; i < BUMPS; i++) {
        pthread_mutex_lock(&m);                       // ENTER critical section
        counter++;                                    //   shared state
        pthread_mutex_unlock(&m);                     // LEAVE critical section
    }
    return NULL;
}

int main(void) {
    pthread_t t[N_THREADS];
    for (int i = 0; i < N_THREADS; i++)
        pthread_create(&t[i], NULL, bumper, NULL);
    for (int i = 0; i < N_THREADS; i++)
        pthread_join(t[i], NULL);

    /* With the lock, counter is always N_THREADS * BUMPS.
     * Comment out the lock/unlock and you'll see a smaller, random-looking
     * number — that's the race. */
    printf("counter = %ld (expected %d)\n",
           counter, N_THREADS * BUMPS);

    pthread_mutex_destroy(&m);
    return 0;
}

/* =============================================================================
 * COMMON MISTAKES (and how they look)
 *
 *   - "Sometimes the answer is wrong."             -> missing lock somewhere.
 *   - "Program hangs forever."                     -> forgot an unlock, or two
 *                                                     locks in opposite orders
 *                                                     (deadlock).
 *   - "Slow when many threads."                    -> critical section too big;
 *                                                     do work outside the lock.
 *   - "Crashes inside pthread_mutex_lock."         -> used a mutex you already
 *                                                     destroyed, or never init'd.
 * =============================================================================
 */
