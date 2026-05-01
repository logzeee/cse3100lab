/*
 * ============================================================================
 *  READERS-WRITERS PROBLEM (Reader-Preference Solution)
 * ============================================================================
 *
 *  GOAL:
 *    Multiple threads want to access a piece of shared data.
 *      - READERS only LOOK at the data. Many of them can do this at the
 *        same time safely (reading does not corrupt anything).
 *      - WRITERS MODIFY the data. Only ONE writer can run at a time, and
 *        no readers may be active while a writer is running (otherwise a
 *        reader could see half-updated data).
 *
 *  CLASSIC RULES we must enforce:
 *      1. Any number of readers can read simultaneously.
 *      2. Only one writer can write at a time.
 *      3. Readers and writers cannot be active at the same time.
 *
 *  STRATEGY (this version is "reader preference"):
 *      - The FIRST reader to arrive grabs a "write_lock" so writers are
 *        blocked while readers are reading.
 *      - Additional readers just bump a counter and walk in.
 *      - The LAST reader to leave releases the "write_lock" so a writer
 *        can finally run.
 *      - A writer simply takes the "write_lock" exclusively.
 *
 *  Two mutexes are used:
 *      - mutex       : protects the reader_count variable itself
 *                      (because multiple readers can change it at once).
 *      - write_lock  : the actual "no one else may touch the data" lock,
 *                      held either by the group of readers or by one writer.
 * ============================================================================
 */

#include <pthread.h>   // threads, mutexes
#include <stdio.h>     // printf
#include <stdlib.h>    // exit, etc.
#include <unistd.h>    // sleep()

/* ---------- Shared state ---------- */

int shared_data = 0;     // the resource everyone is fighting over
int reader_count = 0;    // how many readers are CURRENTLY inside the
                         // reading section. Critical: this is shared
                         // among all readers, so it MUST be protected.

/* ---------- Synchronization primitives ---------- */

pthread_mutex_t mutex;         // protects reader_count (short critical section)
pthread_mutex_t write_lock;    // gives exclusive access to shared_data
                               // to either ONE writer, or the entire
                               // group of readers as a whole.


/* ============================================================================
 *  READER THREAD
 * ============================================================================
 *  Each reader does, forever:
 *      1. Announce "I'm a reader entering" (entry section).
 *      2. Read the data.
 *      3. Announce "I'm leaving" (exit section).
 * ============================================================================ */
void *reader(void *arg) {
    /*
     * arg is a void* that points to an int (the reader's ID number).
     * We cast it back to int* and dereference to get the actual int.
     * We copy it into a LOCAL variable `id` so each thread has its own
     * private copy that nothing else can change.
     */
    int id = *(int *)arg;

    while (1) {
        /* -------- Entry section --------
         * Goal: safely increment reader_count, and if I'm the FIRST
         * reader, block all writers by grabbing write_lock.
         */
        pthread_mutex_lock(&mutex);     // lock the small "counter" mutex
        reader_count++;                 // one more reader is active
        if (reader_count == 1) {
            // I'm the first reader! No readers were here before me,
            // so writers might still be allowed in. Grab write_lock
            // so no writer can start while readers are reading.
            pthread_mutex_lock(&write_lock);
        }
        pthread_mutex_unlock(&mutex);   // release counter mutex; other
                                        // readers may now also enter

        /* -------- Reading section --------
         * Multiple readers may be in here at the same time. That's OK
         * because reads are non-destructive.
         */
        printf("Reader %d reading: %d\n", id, shared_data);
        sleep(1);   // simulate time spent reading

        /* -------- Exit section --------
         * Goal: safely decrement reader_count, and if I'm the LAST
         * reader to leave, release write_lock so writers can run again.
         */
        pthread_mutex_lock(&mutex);
        reader_count--;
        if (reader_count == 0) {
            // I was the last reader. Let any waiting writer through.
            pthread_mutex_unlock(&write_lock);
        }
        pthread_mutex_unlock(&mutex);

        sleep(1);   // wait a bit before reading again
    }
}


/* ============================================================================
 *  WRITER THREAD
 * ============================================================================
 *  Each writer does, forever:
 *      1. Take write_lock (blocks if anyone — reader or writer — has it).
 *      2. Modify the data.
 *      3. Release write_lock.
 * ============================================================================ */
void *writer(void *arg) {
    int id = *(int *)arg;   // same trick: pull our integer ID out of arg

    while (1) {
        /* -------- Entry section --------
         * Just lock write_lock. If readers are reading, they currently
         * hold this lock (the first reader took it), so we wait. If
         * another writer holds it, we also wait. So this single lock
         * enforces "writers are exclusive AND mutually exclusive with
         * readers."
         */
        pthread_mutex_lock(&write_lock);

        /* -------- Writing section --------
         * We are the only thread (reader or writer) that can touch
         * shared_data right now.
         */
        shared_data++;
        printf("Writer %d wrote: %d\n", id, shared_data);
        sleep(1);   // simulate time spent writing

        /* -------- Exit section -------- */
        pthread_mutex_unlock(&write_lock);

        sleep(2);   // wait longer than a reader so writers don't dominate
    }
}


/* ============================================================================
 *  MAIN
 * ============================================================================ */
int main() {
    pthread_t r[3], w[2];   // 3 reader threads and 2 writer threads.

    /*
     * ------------------------------------------------------------------
     *  WHY `int ids[] = {1, 2, 3, 4, 5};`  ?
     * ------------------------------------------------------------------
     *  pthread_create() can only pass ONE argument to a thread, and only
     *  as a generic `void *` pointer. So we need a place in memory whose
     *  address we can hand to each thread. Each thread will then read
     *  the int stored at that address to learn its own ID.
     *
     *  We could declare 5 separate variables (id1, id2, id3, ...), but
     *  it is cleaner to put them in a single array `ids[]`. We have
     *  3 readers + 2 writers = 5 total threads, so we need 5 IDs:
     *      ids[0] = 1   -> reader 1
     *      ids[1] = 2   -> reader 2
     *      ids[2] = 3   -> reader 3
     *      ids[3] = 4   -> writer 4
     *      ids[4] = 5   -> writer 5
     *
     *  IMPORTANT — why NOT pass `&i` from inside the loop?
     *      A common bug is doing:
     *          for (int i = 0; i < 3; i++)
     *              pthread_create(&r[i], NULL, reader, &i);
     *      Here, every thread receives the SAME pointer (&i). By the
     *      time the threads actually run, `i` has already changed (or
     *      even gone out of scope). All threads end up seeing the same
     *      garbage value. By using `ids[i]`, each thread gets a pointer
     *      to its OWN int slot, which never gets reused.
     *
     *  Also note: since the array lives in main()'s stack and main()
     *  waits on the threads via pthread_join, the array is guaranteed
     *  to still exist for the entire lifetime of the threads. Good.
     * ------------------------------------------------------------------
     */
    int ids[] = {1, 2, 3, 4, 5};

    /* Initialize the two mutexes before any thread starts using them. */
    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&write_lock, NULL);

    /* ---- Spawn 3 readers ----
     * Pass &ids[i] (a pointer to that thread's own ID slot) as the arg.
     * The reader function will cast it back to int* and read its ID.
     */
    for (int i = 0; i < 3; i++)
        pthread_create(&r[i], NULL, reader, &ids[i]);

    /* ---- Spawn 2 writers ----
     * Note we KEEP using ids[i] — this loop's `i` goes 0, 1, but we
     * actually want IDs 4 and 5 (the slots after the 3 readers).
     *
     *   *** THIS IS A SUBTLE BUG IN THE ORIGINAL CODE ***
     *
     * Because `i` restarts at 0 in this loop, the writers receive
     * &ids[0] and &ids[1] — which are 1 and 2 — meaning the writers
     * end up sharing IDs with the readers. To match the comment
     * "writer 4" and "writer 5", you'd want:
     *     pthread_create(&w[i], NULL, writer, &ids[i + 3]);
     * The program still RUNS correctly (the ID is only used for
     * printing), but the printed IDs will collide.
     */
    for (int i = 0; i < 2; i++)
        pthread_create(&w[i], NULL, writer, &ids[i]);

    /* Wait forever for the threads. (These threads loop forever, so
     * pthread_join will actually never return in this program — you
     * stop it with Ctrl+C.) */
    for (int i = 0; i < 3; i++) pthread_join(r[i], NULL);
    for (int i = 0; i < 2; i++) pthread_join(w[i], NULL);

    /* Clean up mutexes (only reached if the threads ever exited). */
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&write_lock);

    return 0;
}

/*
 * ============================================================================
 *  QUICK TRACE OF WHAT HAPPENS
 * ============================================================================
 *  Suppose readers R1, R2, R3 and writer W1 all start:
 *
 *    R1 enters:
 *        lock(mutex)
 *        reader_count = 1   -> first reader, so lock(write_lock)
 *        unlock(mutex)
 *        ... reading ...
 *
 *    R2 enters:
 *        lock(mutex)
 *        reader_count = 2   -> NOT first, skip write_lock
 *        unlock(mutex)
 *        ... reading concurrently with R1 ...
 *
 *    W1 tries to enter:
 *        lock(write_lock)   -> BLOCKED (R1 already holds it)
 *
 *    R1 exits:
 *        lock(mutex)
 *        reader_count = 1   -> not the last reader, keep write_lock
 *        unlock(mutex)
 *
 *    R2 exits:
 *        lock(mutex)
 *        reader_count = 0   -> last reader! unlock(write_lock)
 *        unlock(mutex)
 *
 *    W1 finally proceeds, modifies shared_data, then unlocks.
 *
 *  KEY INVARIANTS:
 *    - At any moment, either:
 *         * 0 or more readers are active and write_lock is held by them, OR
 *         * exactly 1 writer is active and write_lock is held by it, OR
 *         * no one is active and write_lock is free.
 *    - reader_count is only modified inside `mutex`, so it is always
 *      consistent.
 *
 *  KNOWN LIMITATION (WRITER STARVATION):
 *    Because a constant stream of readers keeps `reader_count > 0`,
 *    write_lock might never be released, and writers can starve.
 *    "Writer-preference" or "fair" variants fix this by adding more
 *    locks/turnstiles. This version is the simplest reader-preference
 *    textbook solution.
 * ============================================================================
 */
