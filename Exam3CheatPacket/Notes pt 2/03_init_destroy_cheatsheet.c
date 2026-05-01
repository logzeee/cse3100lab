/* 03_init_destroy_cheatsheet.c
 * EVERY pthread sync object follows the same pattern:
 *   declare -> init -> use -> destroy.
 *
 * This file is a one-page reference of the init/destroy pairs you are
 * expected to know on the exam. Nothing here actually runs anything
 * interesting — it's a cheat sheet. It does compile.
 *
 * gcc -Wall 03_init_destroy_cheatsheet.c -o 03_init_destroy_cheatsheet -lpthread
 *
 * (Compile/run on the class server / Linux. macOS doesn't ship
 *  pthread_barrier_t, so the barrier section won't build there.)
 *
 * =============================================================================
 *  OBJECT             STATIC INITIALIZER              DYNAMIC INIT/DESTROY
 *  ---------------    ----------------------------    -----------------------
 *  pthread_mutex_t    PTHREAD_MUTEX_INITIALIZER       pthread_mutex_init/destroy
 *  pthread_cond_t     PTHREAD_COND_INITIALIZER        pthread_cond_init/destroy
 *  pthread_rwlock_t   PTHREAD_RWLOCK_INITIALIZER      pthread_rwlock_init/destroy
 *  pthread_barrier_t  (no static initializer)         pthread_barrier_init/destroy
 *
 * RULE OF THUMB
 *   - For globals with default attributes, use the STATIC initializer.
 *     One-liner. Can't forget to init.
 *   - For anything inside a struct / on the heap / with custom attributes,
 *     use the DYNAMIC init function. Always pair it with destroy.
 * =============================================================================
 */

#include <pthread.h>
#include <stdio.h>

/* ---------- STATIC INITIALIZERS (globals only, default attrs) ----------- */
pthread_mutex_t  gM  = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t   gC  = PTHREAD_COND_INITIALIZER;
pthread_rwlock_t gRW = PTHREAD_RWLOCK_INITIALIZER;
/* NO `pthread_barrier_t  gB = ...;`  — barriers must be init'd dynamically. */

int main(void) {

    /* ---------- MUTEX (dynamic) ----------
     * 2nd arg = attributes.  NULL = defaults (non-recursive, process-private).
     */
    pthread_mutex_t m;
    pthread_mutex_init(&m, NULL);
    /* use:   pthread_mutex_lock(&m);  ...  pthread_mutex_unlock(&m); */
    pthread_mutex_destroy(&m);

    /* ---------- CONDITION VARIABLE (dynamic) ----------
     * 2nd arg = attributes.  NULL = defaults.
     * Always paired with a mutex when used.
     */
    pthread_cond_t c;
    pthread_cond_init(&c, NULL);
    /* use:   pthread_cond_wait(&c, &m);
     *        pthread_cond_signal(&c);
     *        pthread_cond_broadcast(&c);   */
    pthread_cond_destroy(&c);

    /* ---------- READ/WRITE LOCK (dynamic) ----------
     * Many readers OR one writer.  2nd arg = attributes (NULL = defaults).
     */
    pthread_rwlock_t rw;
    pthread_rwlock_init(&rw, NULL);
    /* use:   pthread_rwlock_rdlock(&rw);   // for reading
     *        pthread_rwlock_wrlock(&rw);   // for writing
     *        pthread_rwlock_unlock(&rw);   // both readers AND writers   */
    pthread_rwlock_destroy(&rw);

    /* ---------- BARRIER ----------
     * No static initializer.  3rd arg = COUNT of threads that must arrive.
     * pthread_barrier_wait blocks until COUNT threads have called it,
     * then ALL are released together. Reusable for the next phase.
     */
    pthread_barrier_t b;
    pthread_barrier_init(&b, NULL, 4);             // wait for 4 threads
    /* use:   pthread_barrier_wait(&b);   */
    pthread_barrier_destroy(&b);

    /* The static-init globals don't strictly need destroying, but doing
     * it is harmless and is a good habit. */
    pthread_mutex_destroy(&gM);
    pthread_cond_destroy(&gC);
    pthread_rwlock_destroy(&gRW);

    printf("everything initialized and torn down cleanly.\n");
    return 0;
}

/* =============================================================================
 * EVERY-PROGRAM CHECKLIST (copy this into your exam scratch paper)
 *
 *   [ ] Declared each pthread_t / mutex / cond / barrier / rwlock.
 *   [ ] Initialized each one (static initializer OR *_init).
 *   [ ] For each pthread_create, there is a matching pthread_join (or
 *       pthread_detach).
 *   [ ] For each lock, there is an unlock on EVERY return path.
 *   [ ] Every cond_wait is inside a while-loop on the predicate.
 *   [ ] Every shared variable is read/written under the right lock.
 *   [ ] Destroyed each sync object once threads are gone.
 *   [ ] Compiled with -lpthread.
 * =============================================================================
 */
