/* =====================================================================
 * pc_explained.c   --   Producer / Consumer with ONE buffer slot
 *
 * This file is a beginner-friendly, exam-prep walkthrough of the classic
 * Producer-Consumer problem in C using POSIX threads (pthreads).
 *
 * It is modeled on the simpler `pc.c` from the Exam3 videos folder, but
 * every step is annotated. Read top-to-bottom like a study guide.
 *
 * Compile:   gcc -Wall -O2 pc_explained.c -o pc_explained -lpthread
 * Run:       ./pc_explained
 * Stop:      Ctrl-C   (this demo loops forever, like the original pc.c)
 *
 * =====================================================================
 *  PART 0  --  THE 30-SECOND MENTAL MODEL
 * =====================================================================
 *
 *  Producer  ->  [ shared buffer ]  ->  Consumer
 *
 *  - The producer makes items and PUTS them into a shared buffer.
 *  - The consumer TAKES items out of that buffer and uses them.
 *  - They run on DIFFERENT threads, so they can step on each other.
 *
 *  Two kinds of "stepping on each other" can happen:
 *    (1) RACE on the buffer itself  -> fix with a mutex.
 *    (2) Producer tries to put when buffer is FULL,
 *        or Consumer tries to take when buffer is EMPTY
 *        -> fix with condition variables (pthread_cond_t).
 *
 *  So every producer/consumer solution has the same 3 ingredients:
 *      - a buffer (array)         -> "what we share"
 *      - a mutex                  -> "who is allowed to touch it"
 *      - one or two cond vars     -> "wake me up when state changes"
 *
 * =====================================================================
 *  PART 1  --  VOCAB CHEAT SHEET (memorize these)
 * =====================================================================
 *
 *  pthread_t                -> handle for a thread (like a PID for a thread)
 *  pthread_create(...)      -> start a new thread that runs a function
 *  pthread_join(...)        -> wait for a thread to finish
 *
 *  pthread_mutex_t          -> a LOCK. Only ONE thread can hold it at a time.
 *  pthread_mutex_lock(m)    -> "I want exclusive access. Block if needed."
 *  pthread_mutex_unlock(m)  -> "I'm done, others may go."
 *
 *  pthread_cond_t           -> a WAITING ROOM tied to some condition.
 *  pthread_cond_wait(c, m)  -> "Atomically: unlock m, sleep on c.
 *                              When woken, re-lock m before returning."
 *  pthread_cond_signal(c)   -> "Wake ONE waiter on c (if any)."
 *  pthread_cond_broadcast(c)-> "Wake ALL waiters on c."
 *
 *  GOLDEN RULES:
 *    R1.  Always hold the mutex while reading/writing shared state.
 *    R2.  Always wait inside a `while (...)` loop (NOT `if`). Why?
 *         Spurious wakeups exist, and another thread may have changed
 *         the state again before you got the lock. Re-check.
 *    R3.  You must hold the mutex when calling pthread_cond_wait.
 *         You should normally hold it when calling cond_signal too.
 *    R4.  signal() wakes ONE thread. broadcast() wakes EVERY waiter.
 *         Use broadcast when more than one waiter could legitimately
 *         proceed (we'll see this matters in the divisor-version
 *         file `cond-div.c`).
 *
 * =====================================================================
 *  PART 2  --  WHY DO WE EVEN NEED A CONDITION VARIABLE?
 * =====================================================================
 *
 *  Naive idea: "Just use a mutex and busy-wait."
 *
 *      while (1) {
 *          pthread_mutex_lock(&m);
 *          if (count != BUFFER_SIZE) { put item; break; }
 *          pthread_mutex_unlock(&m);    // try again later... burns CPU
 *      }
 *
 *  This works but it WASTES the CPU spinning. A condition variable lets
 *  the thread go to SLEEP until someone tells it "the world changed,
 *  check again." That's much more efficient and is the standard pattern.
 *
 * =====================================================================
 *  PART 3  --  COMMON BUGS THAT SHOW UP ON EXAMS
 * =====================================================================
 *
 *  BUG A: Using `if` instead of `while` around cond_wait.
 *         -> can consume from an empty buffer because of spurious wakeups,
 *            or because another consumer raced you to it.
 *
 *  BUG B: Forgetting to hold the mutex while calling cond_wait.
 *         -> undefined behavior. cond_wait MUST be called with the lock.
 *
 *  BUG C: Forgetting to unlock before exiting a path.
 *         -> deadlock: nobody else can ever take the lock.
 *
 *  BUG D: Signaling the wrong cond var (e.g. signaling consumers when
 *         you should be signaling producers).
 *         -> all threads sleep forever. Looks like the program "hangs."
 *
 *  BUG E: Using signal() when you should use broadcast().
 *         -> only one waiter wakes, but several could have made progress.
 *            Causes "stuck" behavior in multi-consumer programs (see
 *            cond-div-1.c which deliberately shows this bug).
 *
 *  BUG F: Reading shared variables without the lock.
 *         -> classic race. The compiler/CPU may even reorder things.
 *
 * =====================================================================
 *  PART 4  --  THE CODE
 * ===================================================================== */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* --------------------------------------------------------------------
 * Shared state.
 *
 * BUFFER_SIZE = 1 means there is room for exactly ONE item at a time.
 * That keeps things simple: the buffer is either "full" (count == 1)
 * or "empty" (count == 0). Once you understand size 1, scaling up to
 * a circular buffer (size 5, 10, ...) is just bookkeeping with `in`
 * and `out` indices, see pc_variation2_buffer5.c for that.
 * ------------------------------------------------------------------ */
#define BUFFER_SIZE 1

int buffer[BUFFER_SIZE];   /* the shared array */
int count = 0;             /* how many items currently in the buffer */

/* --------------------------------------------------------------------
 * The synchronization toolkit.
 *
 *   - mutex        : guards access to `buffer` and `count`.
 *   - cond_producer: producer waits here when buffer is FULL.
 *                    consumer signals it after taking an item out.
 *   - cond_consumer: consumer waits here when buffer is EMPTY.
 *                    producer signals it after putting an item in.
 *
 * Naming tip: name the cond var after WHO WAITS on it. That makes the
 * code read like English: "wake the producer", "wake the consumer".
 * ------------------------------------------------------------------ */
pthread_mutex_t mutex;
pthread_cond_t  cond_producer;
pthread_cond_t  cond_consumer;

/* --------------------------------------------------------------------
 * Helper: initialize all sync objects with default attributes.
 *
 * You can also use the static initializers
 *     PTHREAD_MUTEX_INITIALIZER  /  PTHREAD_COND_INITIALIZER
 * (see pc_variation1_buffer1.c). The `_init` form is required when
 * the object is heap-allocated or has non-default attributes.
 * ------------------------------------------------------------------ */
void initialize_sync_objects(void) {
    if (pthread_mutex_init(&mutex, NULL) != 0) {
        perror("Mutex initialization failed");
        exit(EXIT_FAILURE);
    }
    if (pthread_cond_init(&cond_producer, NULL) != 0) {
        perror("Producer condition variable initialization failed");
        exit(EXIT_FAILURE);
    }
    if (pthread_cond_init(&cond_consumer, NULL) != 0) {
        perror("Consumer condition variable initialization failed");
        exit(EXIT_FAILURE);
    }
}

/* Always destroy what you initialized. Good hygiene, and the OS may
 * leak resources if you don't. */
void cleanup_sync_objects(void) {
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_producer);
    pthread_cond_destroy(&cond_consumer);
}

/* =====================================================================
 *  PART 5  --  THE PRODUCER
 * =====================================================================
 *
 * Pseudocode:
 *
 *     loop:
 *         make an item
 *         LOCK
 *         while (buffer is full) WAIT on cond_producer
 *         put item in buffer
 *         SIGNAL cond_consumer    ("hey consumer, food's ready")
 *         UNLOCK
 *
 * Why the WAIT pattern looks weird:
 *   pthread_cond_wait does THREE atomic things:
 *     1. release the mutex
 *     2. put this thread to sleep on the cond var
 *     3. when woken, re-acquire the mutex BEFORE returning
 *   That's why you must already hold the mutex to call it.
 * ===================================================================== */
void *producer(void *arg) {
    (void)arg;             /* unused, suppress warning */
    int item;

    while (1) {
        /* 1. Produce something. This is done OUTSIDE the lock because
         *    creating the item doesn't touch shared state. Keep
         *    critical sections SHORT. */
        item = rand() % 100;

        /* 2. Enter the critical section. */
        pthread_mutex_lock(&mutex);

        /* 3. WAIT WHILE the buffer is full.
         *    Use `while`, not `if`! See BUG A above. */
        while (count == BUFFER_SIZE) {
            /* Atomically: unlock mutex, sleep on cond_producer.
             * When some consumer signals cond_producer, we wake
             * AND re-acquire the mutex before returning here. */
            pthread_cond_wait(&cond_producer, &mutex);
        }

        /* 4. Now we are guaranteed:
         *      - we hold the mutex
         *      - count < BUFFER_SIZE  (there is room)
         *    So it's safe to insert. */
        buffer[count] = item;
        count++;
        printf("Produced: %d\n", item);

        /* 5. Tell the consumer something is available.
         *    With one consumer, signal() is enough.
         *    With many consumers waiting, you'd often broadcast(). */
        pthread_cond_signal(&cond_consumer);

        /* 6. Leave the critical section. */
        pthread_mutex_unlock(&mutex);

        /* 7. Simulate work outside the critical section.
         *    NEVER sleep while holding the mutex -- that blocks
         *    everyone else. */
        sleep(1);
    }
    return NULL;
}

/* =====================================================================
 *  PART 6  --  THE CONSUMER
 * =====================================================================
 *
 * Mirror image of the producer:
 *
 *     loop:
 *         LOCK
 *         while (buffer is empty) WAIT on cond_consumer
 *         take item from buffer
 *         SIGNAL cond_producer    ("hey producer, room is free")
 *         UNLOCK
 *         use the item
 * ===================================================================== */
void *consumer(void *arg) {
    (void)arg;
    int item;

    while (1) {
        pthread_mutex_lock(&mutex);

        /* WAIT WHILE the buffer is empty. Same `while` rule. */
        while (count == 0) {
            printf("consumer: buffer empty, going to sleep\n");
            pthread_cond_wait(&cond_consumer, &mutex);
            printf("consumer: woken up by signal\n");
        }

        /* We hold the mutex AND count > 0, so removing is safe. */
        item = buffer[count - 1];
        count--;
        printf("Consumed: %d\n", item);

        /* Tell the producer there is now room. */
        pthread_cond_signal(&cond_producer);

        pthread_mutex_unlock(&mutex);

        /* (No sleep here: the consumer is fast. The producer's
         *  sleep(1) controls the overall pace.) */
    }
    return NULL;
}

/* =====================================================================
 *  PART 7  --  MAIN
 * =====================================================================
 *
 * pthread_create signature:
 *     int pthread_create(pthread_t *tid,
 *                        const pthread_attr_t *attr,   // NULL = defaults
 *                        void *(*start_routine)(void*),// the function
 *                        void *arg);                   // passed to it
 *
 * The function MUST have signature `void *f(void *)`. If you don't need
 * an argument, pass NULL and ignore it inside.
 *
 * pthread_join blocks the caller until the named thread has finished.
 * In this demo the threads loop forever, so join() never returns --
 * the program ends with Ctrl-C.
 * ===================================================================== */
int main(void) {
    pthread_t prod, cons;

    initialize_sync_objects();

    pthread_create(&prod, NULL, producer, NULL);
    pthread_create(&cons, NULL, consumer, NULL);

    pthread_join(prod, NULL);
    pthread_join(cons, NULL);

    cleanup_sync_objects();
    return 0;
}

/* =====================================================================
 *  PART 8  --  EXAM-STYLE Q & A
 * =====================================================================
 *
 *  Q1. Why two condition variables instead of one?
 *  A1. With one cond var you risk waking the WRONG thread. Example:
 *      a producer and a consumer are both blocked. A new producer
 *      finishes and signals "the one cond var" -- if it wakes another
 *      producer, that producer just goes back to sleep (buffer still
 *      full) and the consumer never gets woken. Two cond vars gives
 *      you precise targeting: "wake a producer" vs "wake a consumer".
 *
 *  Q2. Why `while` and not `if` around cond_wait?
 *  A2. Three reasons:
 *        (a) Spurious wakeups: pthreads is allowed to wake you for no
 *            reason. The standard explicitly permits this.
 *        (b) Multiple waiters: another thread may have raced you and
 *            changed the state back (e.g. another consumer grabbed
 *            the item before you re-acquired the lock).
 *        (c) Broadcast: if someone broadcasts, every waiter wakes,
 *            but only some of them can legitimately proceed.
 *
 *  Q3. signal vs broadcast?
 *  A3. signal()    : exactly one waiter wakes (if any). Cheaper.
 *      broadcast() : every waiter wakes, then they fight for the lock
 *                    one by one and re-check their predicate.
 *      Use broadcast when "more than one waiter could now make
 *      progress" (different predicates, multiple consumers, etc.).
 *      cond-div.c uses broadcast because the producer doesn't know
 *      WHICH consumer the data is for; cond-div-1.c "breaks" by using
 *      signal in the same situation -- compare them side by side.
 *
 *  Q4. What is the critical section here?
 *  A4. Everything between pthread_mutex_lock and pthread_mutex_unlock.
 *      Keep it as small as possible: do CPU/IO work outside the lock.
 *
 *  Q5. Will this deadlock?
 *  A5. No. Each thread holds AT MOST one lock and never holds it
 *      across an operation that needs another lock. The classic
 *      deadlock recipe ("hold A, ask for B; hold B, ask for A") does
 *      not appear here.
 *
 *  Q6. Could items be consumed out of order?
 *  A6. Not with one producer + one consumer + buffer size 1. With
 *      multiple producers/consumers and a circular buffer, FIFO order
 *      is preserved as long as you always insert at `in` and remove
 *      at `out` (see pc_variation2_buffer5.c).
 *
 *  Q7. What if the producer is much faster than the consumer?
 *  A7. The producer will repeatedly find count==BUFFER_SIZE, call
 *      cond_wait, and block until the consumer signals. The system
 *      naturally throttles to the consumer's rate. The opposite case
 *      throttles symmetrically.
 *
 *  Q8. What changes for a bigger buffer?
 *  A8. Replace the single slot with a CIRCULAR buffer:
 *          buffer[in] = item;  in = (in + 1) % BUFFER_SIZE;  count++;
 *          item = buffer[out]; out = (out + 1) % BUFFER_SIZE; count--;
 *      The mutex/cond logic is IDENTICAL. Only the indexing changes.
 *
 *  Q9. How do you cleanly stop the threads (instead of looping forever)?
 *  A9. Add a shared "done" flag (protected by the same mutex), and a
 *      sentinel value or counter. After the producer finishes, it
 *      sets done = 1 and broadcasts so any sleeping consumer can wake
 *      up, see "done && count == 0", and return. See
 *      pc_variation3_buffer10_barrier.c, where `producers_done` plays
 *      exactly this role.
 *
 * ===================================================================== */
