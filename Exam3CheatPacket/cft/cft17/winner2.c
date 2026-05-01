/*
 * winner2.c — Heavily commented version of winner.c
 * ---------------------------------------------------
 * PURPOSE:
 *   Simulate `n` game players, each running on its own thread. Every player:
 *     1) plays a "fun_game" and gets a score,
 *     2) writes (id, score) into a shared array,
 *     3) waits for ALL other players to finish writing,
 *     4) reads the array to find the highest score,
 *     5) if it owns the highest score, it prints that it's the winner.
 *
 *   The assignment exercises TWO synchronization primitives:
 *     - pthread_rwlock_t  : a reader/writer lock, used to protect the shared
 *                           `scores[]` array and the shared `count` index.
 *     - pthread_barrier_t : a barrier, used to make every thread pause until
 *                           ALL threads have written their score. Without
 *                           this, a fast thread could read `scores[]` before
 *                           slower threads have stored their results.
 *
 *   Assumption from the spec: there is exactly one winner (no ties).
 */

#include <pthread.h>   // threads, mutexes, rwlocks, barriers
#include <stdio.h>     // printf
#include <stdlib.h>    // atoi
#include <unistd.h>    // POSIX API (not strictly required here)
#include <assert.h>    // assert()
#include <string.h>    // unused here, but commonly included

#define NUM 10000      // hard upper bound on number of player threads

/*
 * Global shared state.
 * Because multiple threads will touch `count` and `scores[]`, we MUST
 * protect them with a lock. We use a read/write lock (rwlock) because:
 *   - Writes happen ONCE per thread (storing its own score).  -> wrlock
 *   - Reads happen ONCE per thread (scanning for the max).    -> rdlock
 * A rwlock allows many readers to scan concurrently, which is faster
 * than forcing them through a plain mutex one-at-a-time.
 */
int count = 0;                       // next free slot in scores[]; shared!
struct player_data
{
    int id;     // which player produced this score
    int score;  // the score value returned by fun_game()
};
struct player_data scores[NUM];      // shared array of all results

pthread_barrier_t barrier;           // makes every thread wait for all peers
pthread_rwlock_t  rwlock;            // protects `count` and `scores[]`

/*
 * Each thread receives a pointer to one of these so it knows its player id.
 * We use a per-thread struct (instead of just passing an int*) so the same
 * pattern scales if you ever need to pass more data per player.
 */
struct thread_data
{
    int id;
};

/*
 * fun_game() — a deterministic "game" based on the Collatz sequence.
 *   Given a starting number n, repeatedly:
 *     - if n is even, divide it by 2
 *     - if n is odd,  set n = 3n + 1
 *   Count how many steps until n drops to 1 (or grows past 100000, which
 *   acts as a safety cap so this can't run forever).
 *   The number of steps becomes the player's "score".
 *   Different starting ids produce different scores -> a natural winner.
 */
int fun_game(int n)
{
    int cnt = 0;
    while (n > 1 && n < 100000)
    {
        if (n % 2 == 0) n /= 2;     // even step
        else            n = 3*n + 1;// odd step
        cnt++;                      // count how many steps we took
    }
    return cnt;
}

/*
 * player_func() — the function each player thread runs.
 * Lifecycle of one thread:
 *   (A) play the game and compute MY score (no shared state — no lock needed)
 *   (B) WRITE my (id, score) into the shared scores[] array  -> need wrlock
 *   (C) WAIT at the barrier until every other thread is also past (B)
 *   (D) READ the whole scores[] array to find the max         -> need rdlock
 *   (E) if I am the winner, print the announcement
 */
void* player_func(void* threadarg)
{
    /* Recover this thread's private data (its player id). */
    struct thread_data* my_data = (struct thread_data*) threadarg;
    int my_id = my_data->id;

    /* (A) Play the game on my own — purely local work, no synchronization. */
    int score = fun_game(my_id);

    /*
     * (B) WRITE PHASE — protected by an EXCLUSIVE write lock.
     * We must lock here because two writers could otherwise:
     *   - both read the same `count` value,
     *   - both write to scores[count],
     *   - both increment count -> one score is lost (race condition).
     * Only one thread at a time is allowed inside this critical section.
     */
    pthread_rwlock_wrlock(&rwlock);   // <-- acquire WRITE lock
    scores[count].id    = my_id;      // store my id into the next free slot
    scores[count].score = score;      // store my score into the next free slot
    count++;                          // advance the shared write index
    pthread_rwlock_unlock(&rwlock);   // <-- release the lock ASAP

    /*
     * (C) BARRIER — wait until ALL n threads have reached this point.
     * Why we need this:
     *   Without the barrier, a fast thread could enter the read phase below
     *   before slower threads have written their scores. It would then think
     *   it's the winner based on incomplete data. The barrier guarantees
     *   that scores[] is fully populated before anyone scans it.
     *   The barrier was initialized in main() with a count of `n`, so it
     *   releases all threads simultaneously once the n-th one arrives.
     */
    pthread_barrier_wait(&barrier);

    /*
     * (D) READ PHASE — protected by a SHARED read lock.
     * Many threads can hold the read lock at the same time (that's the whole
     * point of a rwlock). No one can be writing right now anyway because
     * every thread already finished (B) before the barrier released them,
     * but holding the read lock is still good defensive practice and makes
     * the code's intent obvious: "I'm only reading."
     */
    pthread_rwlock_rdlock(&rwlock);   // <-- acquire READ lock (shared)

    int max_score = scores[0].score;  // start by assuming player[0] is best
    int winner    = scores[0].id;
    for (int i = 0; i < count; i++)   // scan the entire results array
    {
        if (scores[i].score > max_score)
        {
            max_score = scores[i].score;  // found a higher score
            winner    = scores[i].id;     // remember who got it
        }
    }
    int num = count;                  // snapshot count while holding the lock

    pthread_rwlock_unlock(&rwlock);   // <-- release the read lock

    /*
     * (E) Only the actual winner prints. Every other thread silently exits.
     * Because the spec says there's exactly one winner, exactly one line
     * gets printed regardless of how many threads ran.
     */
    if (winner == my_id)
        printf("Out of %d players, the winner is %d, with score %d.\n",
               num, my_id, score);

    pthread_exit(NULL);               // clean per-thread exit
}

/*
 * main() — sets everything up, launches threads, joins them, then cleans up.
 * Per the original assignment: do NOT change the logic of main().
 */
int main(int argc, char *argv[])
{
    /* Expect exactly one argument: the number of players n. */
    if (argc != 2)
    {
        printf("Usage: %s n(100 - 10000)\n", argv[0]);
        return -1;
    }
    int n = atoi(argv[1]);
    assert(n >= 100 && n <= 10000);   // enforce the allowed range

    pthread_t threads[NUM];           // handles for each player thread

    /*
     * Initialize the barrier with `n` — meaning n threads must call
     * pthread_barrier_wait() before any of them are released.
     */
    pthread_barrier_init(&barrier, NULL, n);

    /* Per-thread data records (gives each thread its own id without races). */
    struct thread_data thread_data_array[NUM];

    /*
     * NOTE: This `pthread_rwlock_t rwlock;` shadows the GLOBAL `rwlock`.
     * The threads use the GLOBAL one (declared at file scope), so this
     * local declaration here is essentially unused. Initializing the
     * GLOBAL rwlock would normally be done with pthread_rwlock_init(&rwlock,...)
     * BEFORE creating threads. The original assignment code is preserved
     * exactly as-is per the instructions; just be aware of the subtlety.
     */
    pthread_rwlock_t rwlock;
    pthread_rwlock_init(&rwlock, NULL);

    /*
     * Spawn n player threads. Each one gets a pointer to its own slot
     * in thread_data_array[] so that the `id` field cannot be overwritten
     * while the new thread is reading it.
     */
    for (int i = 0; i < n; i++)
    {
        thread_data_array[i].id = i + 1;   // ids are 1-based per the spec
        pthread_create(&threads[i], NULL, (void*)player_func,
                       &thread_data_array[i]);
    }

    /* Wait for every player thread to finish before tearing things down. */
    for (int i = 0; i < n; i++)
    {
        pthread_join(threads[i], NULL);
    }

    /* Always release OS resources held by sync primitives when done. */
    pthread_barrier_destroy(&barrier);
    pthread_rwlock_destroy(&rwlock);

    return 0;
}
