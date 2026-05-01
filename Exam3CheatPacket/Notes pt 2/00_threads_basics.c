/* 00_threads_basics.c
 * THREAD LIFECYCLE: create -> run -> join.
 * The two functions you cannot live without: pthread_create, pthread_join.
 *
 * gcc -Wall 00_threads_basics.c -o 00_threads_basics -lpthread
 *
 * =============================================================================
 * THE 4 THINGS YOU ALWAYS DO WITH A THREAD
 *
 *   1. DECLARE       pthread_t t;                  // a "handle" to the thread
 *   2. CREATE        pthread_create(&t, NULL, fn, arg);
 *   3. RUN           the function `fn` runs concurrently with main()
 *   4. JOIN          pthread_join(t, NULL);        // main waits for it to end
 *
 * If you skip JOIN, main() may exit while the thread is still running ->
 * the OS kills the thread mid-work. Always join (or pthread_detach).
 *
 * =============================================================================
 * pthread_create SIGNATURE (memorize the slot order)
 *
 *   int pthread_create(pthread_t       *thread,    // OUT: filled with handle
 *                      pthread_attr_t  *attr,      // NULL = defaults
 *                      void *         (*start)(void *),  // function to run
 *                      void           *arg);       // pointer passed to start()
 *
 *   Returns 0 on success, errno-style number on failure.
 *
 * pthread_join SIGNATURE
 *
 *   int pthread_join(pthread_t thread, void **retval);
 *      retval = NULL if you don't care about the return value.
 * =============================================================================
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define N 4

/* The thread function MUST have this exact shape:
 *      void *name(void *arg);
 * Anything else and pthread_create won't accept it.
 */
void *worker(void *arg) {
    int id = *(int *)arg;                 // unpack the int we passed in
    printf("hello from thread %d\n", id);
    return NULL;                          // could also: pthread_exit(NULL);
}

int main(void) {
    pthread_t t[N];
    int       id[N];                      // <-- separate slot per thread!

    /* COMMON BUG: passing &i from inside the loop.
     *   for (int i = 0; i < N; i++)
     *       pthread_create(&t[i], 0, worker, &i);   // BAD
     * All threads share the same address &i, and i keeps changing. By the time
     * the threads read it, they may all see the same value (often N).
     * FIX: give each thread its own storage, like id[i] below.
     */
    for (int i = 0; i < N; i++) {
        id[i] = i + 1;
        if (pthread_create(&t[i], NULL, worker, &id[i]) != 0) {
            perror("pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    /* Always join every thread you create (or detach it).
     * Order doesn't matter — join just blocks until that specific thread ends.
     */
    for (int i = 0; i < N; i++)
        pthread_join(t[i], NULL);

    printf("main done\n");
    return 0;
}

/* =============================================================================
 * QUICK REFERENCE
 *
 *   pthread_create(&t, NULL, fn, arg)   start a thread
 *   pthread_join(t, &retval)            wait for it; retval can be NULL
 *   pthread_exit(retval)                end THIS thread early (== return retval)
 *   pthread_self()                      get my own pthread_t
 *   pthread_detach(t)                   "I won't join you" — OS reaps it
 *
 *   Compile with -lpthread.  Always.
 *
 *   Passing data IN  : cast a pointer to (void *).  Make sure the storage
 *                      stays alive for the life of the thread (heap or
 *                      a per-thread stack slot in main, NOT a loop variable).
 *   Passing data OUT : return a pointer from the thread fn, recover via the
 *                      second arg of pthread_join.
 * =============================================================================
 */
