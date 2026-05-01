/* 04_rwlock_and_barrier_basics.c
 * Two more building blocks the exam likes:
 *   - pthread_rwlock_t  : "many readers OR one writer"
 *   - pthread_barrier_t : "everyone wait here until N have arrived"
 *
 * gcc -Wall 04_rwlock_and_barrier_basics.c -o 04_rwlock_and_barrier_basics -lpthread
 *
 * Compile/run on the class server (Linux). macOS doesn't ship
 * pthread_barrier_t in its system headers. The `winner.c` problem
 * (cft17) is a textbook combo of these two.
 *
 * =============================================================================
 * RWLOCK — when to use it
 *
 *   Use this instead of a plain mutex when:
 *     - You have a SHARED data structure that is READ way more than written.
 *     - Many readers can happily look at the same data at once.
 *     - But a writer needs the structure ALL TO ITSELF.
 *
 *   THE 4 CALLS YOU ALWAYS USE
 *     pthread_rwlock_init(&rw, NULL);
 *     pthread_rwlock_rdlock(&rw);    // shared (read) lock — many at once
 *     pthread_rwlock_wrlock(&rw);    // exclusive (write) lock — one only
 *     pthread_rwlock_unlock(&rw);    // SAME unlock for both rd and wr
 *     pthread_rwlock_destroy(&rw);
 *
 *   GOTCHAS
 *     - One unlock function for both kinds. Don't go looking for rd_unlock.
 *     - A thread that already holds the read lock CANNOT upgrade to a write
 *       lock — that would deadlock. Drop the read lock first.
 *     - Writers can starve if readers keep arriving (depends on platform).
 *
 * =============================================================================
 * BARRIER — when to use it
 *
 *   "I need EVERY thread to finish phase 1 before ANY thread starts phase 2."
 *   Classic uses: warmup, parallel-step algorithms, simulations, the
 *   `winner.c` "wait for everyone to record their score" pattern.
 *
 *   THE 3 CALLS YOU ALWAYS USE
 *     pthread_barrier_init(&b, NULL, COUNT);   // COUNT = threads that must arrive
 *     pthread_barrier_wait(&b);                // each thread calls this
 *     pthread_barrier_destroy(&b);
 *
 *   What pthread_barrier_wait does:
 *     - Increments an internal counter under the hood.
 *     - If you are NOT the COUNT-th arrival, you BLOCK.
 *     - The COUNT-th arrival WAKES EVERYONE and the barrier resets so it can
 *       be used again for the next phase.
 *
 *   ONE return value of pthread_barrier_wait is PTHREAD_BARRIER_SERIAL_THREAD;
 *   exactly one of the threads gets that. Useful when you want one-of-them to
 *   do "the cleanup of phase 1" before phase 2 begins.
 * =============================================================================
 */

#include <pthread.h>
#include <stdio.h>

#define N 4

int               scores[N];
pthread_rwlock_t  rw;
pthread_barrier_t bar;

void *player(void *arg) {
    int id = *(int *)arg;

    /* PHASE 1: each player writes their score.
     * Writing the shared array — exclusive lock. */
    pthread_rwlock_wrlock(&rw);
    scores[id] = id * 10 + 7;                       // pretend "score"
    pthread_rwlock_unlock(&rw);

    /* WAIT for everyone to finish phase 1 before reading. */
    pthread_barrier_wait(&bar);

    /* PHASE 2: everyone reads the array to find the max.
     * Reading only — shared lock; all N threads can read at the same time. */
    pthread_rwlock_rdlock(&rw);
    int best_id = 0, best = scores[0];
    for (int i = 1; i < N; i++)
        if (scores[i] > best) { best = scores[i]; best_id = i; }
    pthread_rwlock_unlock(&rw);

    if (id == best_id)
        printf("player %d wins with %d\n", id, best);
    return 0;
}

int main(void) {
    pthread_t t[N];
    int       id[N];

    pthread_rwlock_init(&rw, NULL);
    pthread_barrier_init(&bar, NULL, N);

    for (int i = 0; i < N; i++) { id[i] = i;
        pthread_create(&t[i], 0, player, &id[i]); }
    for (int i = 0; i < N; i++) pthread_join(t[i], 0);

    pthread_barrier_destroy(&bar);
    pthread_rwlock_destroy(&rw);
    return 0;
}

/* =============================================================================
 * COMPARE WITH THE OTHER TOOLS
 *
 *   mutex      one-at-a-time access, no waiting-for-state
 *   rwlock     like a mutex, but lets readers share
 *   cond var   "sleep until a predicate becomes true" (paired with mutex)
 *   barrier    "all threads must arrive before any moves on"
 *
 * If you find yourself doing `while (count != N) pthread_cond_wait(...)`
 * just to sync phases, you probably want a barrier instead.
 * =============================================================================
 */
