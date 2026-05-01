/* 02_condvar_basics.c
 * CONDITION VARIABLES — the part the exam loves to ask about.
 *
 * A cond var lets a thread SLEEP until "something becomes true," without
 * spinning the CPU. It is ALWAYS used with a mutex.
 *
 * gcc -Wall 02_condvar_basics.c -o 02_condvar_basics -lpthread
 *
 * =============================================================================
 * THE GOLDEN RULES (memorize these — exam gold)
 *
 *   1. The mutex MUST already be locked before you call pthread_cond_wait.
 *
 *   2. ALWAYS wait inside a `while`, NEVER an `if`:
 *
 *          while (!predicate)
 *              pthread_cond_wait(&cv, &m);
 *
 *      Two reasons:
 *        (a) SPURIOUS WAKEUPS — POSIX explicitly allows cond_wait to return
 *            with no signal sent. Recheck the predicate.
 *        (b) STOLEN WAKEUPS — between signal and you re-acquiring the mutex,
 *            another thread may have already eaten the condition.
 *
 *   3. cond_wait's magic (atomic):
 *        - UNLOCK the mutex
 *        - SLEEP until someone signals/broadcasts
 *        - RE-LOCK the mutex before returning to your code
 *      You never see the unlocked window. You always wake up holding the lock.
 *
 *   4. SIGNAL vs BROADCAST:
 *        signal     -> wake ONE waiter      (use when only one can proceed)
 *        broadcast  -> wake ALL waiters     (use when condition lets many
 *                                            proceed, OR when you change state
 *                                            that several different waiters
 *                                            care about, OR for shutdown)
 *
 *   5. SIGNAL IS NOT REMEMBERED.
 *      If nobody is waiting, the signal is LOST. That's why you check the
 *      predicate yourself; you don't rely on the signal being delivered.
 *
 *   6. You may signal/broadcast from EITHER inside or outside the lock.
 *      Inside the lock is the safe default; do that unless you have a reason.
 *
 * =============================================================================
 * THE STANDARD WAIT/SIGNAL SKELETON
 *
 *   WAITER:                           SIGNALER:
 *     LOCK(m);                          LOCK(m);
 *     while (!ready)                    ready = 1;
 *         WAIT(cv, m);                  SIGNAL(cv);     // or BROADCAST
 *     // ready is true here             UNLOCK(m);
 *     ... use shared state ...
 *     UNLOCK(m);
 *
 * If you remember nothing else, remember the WAITER block above.
 * =============================================================================
 */

#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

int             ready = 0;                         // shared predicate
pthread_mutex_t m  = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cv = PTHREAD_COND_INITIALIZER;

void *waiter(void *arg) {
    int id = *(int *)arg;

    pthread_mutex_lock(&m);
    while (!ready)                                 // <-- WHILE, not IF
        pthread_cond_wait(&cv, &m);                // sleeps WITHOUT holding m
    /* When we get here: m is locked AND ready == 1. */
    printf("waiter %d woke up, ready=%d\n", id, ready);
    pthread_mutex_unlock(&m);
    return 0;
}

void *signaler(void *_) {
    (void)_;
    sleep(1);                                      // pretend work
    pthread_mutex_lock(&m);
    ready = 1;
    pthread_cond_broadcast(&cv);                   // wake ALL waiters
    pthread_mutex_unlock(&m);
    return 0;
}

int main(void) {
    pthread_t w[3], s;
    int id[3] = {1, 2, 3};

    for (int i = 0; i < 3; i++)
        pthread_create(&w[i], 0, waiter, &id[i]);
    pthread_create(&s, 0, signaler, 0);

    for (int i = 0; i < 3; i++)
        pthread_join(w[i], 0);
    pthread_join(s, 0);

    pthread_mutex_destroy(&m);
    pthread_cond_destroy(&cv);
    return 0;
}

/* =============================================================================
 * IF YOU DO IT WRONG...
 *
 *   - Used `if` instead of `while`?
 *       Sometimes the thread proceeds when the predicate is still false.
 *       Crashes, wrong answers, intermittent failures.
 *
 *   - Forgot to lock before cond_wait?
 *       Undefined behavior. On some systems it just deadlocks; others crash.
 *
 *   - Signaled before any thread was waiting?
 *       Signal is dropped. The waiter that arrives later sleeps forever
 *       UNLESS its predicate already became true (which it should, if you
 *       set the shared flag before signaling — see signaler() above).
 *
 *   - Used signal() when many waiters could proceed?
 *       Only ONE wakes up. The others sleep until next signal. Use broadcast.
 *
 *   - Changed shared state OUTSIDE the lock and then signaled?
 *       Race: a waiter may miss the change. Update state INSIDE the lock.
 * =============================================================================
 */
