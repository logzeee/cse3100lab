# pthread Quick Notes — Init / Args / Cleanup

A simple cheat sheet for threads, mutexes, condition variables, barriers, and
read-write locks in C (POSIX threads / `pthread`).

> Always `#include <pthread.h>` and link with `-lpthread` (or `-pthread`):
> `gcc myprog.c -o myprog -pthread`

---

## 1. Threads — `pthread_t`

### Declare
```c
pthread_t tid;            // one thread
pthread_t pool[5];        // an array of threads
```

### Create
```c
int pthread_create(
    pthread_t *where_to_store_thread_id,       // use &tid or &threads[i]
    const pthread_attr_t *thread_settings,     // usually NULL = default settings
    void *(*function_i_want_thread_to_run)(void *),
    void *argument_i_want_to_give_that_function
);
```

Example:
```c
pthread_create(&tid, NULL, function_i_want_thread_to_run, &my_argument);
```

The thread function MUST have this exact signature:
```c
void *function_i_want_thread_to_run(void *arg) { ... return NULL; }
```

Easy-English version:
```c
pthread_create(
    &tid,                            // put the new thread ID into tid
    NULL,                            // use normal/default thread settings
    function_i_want_thread_to_run,   // function the thread starts running
    &my_argument                     // address of the data I want to pass in
);
```

### Join (wait for it to finish)
```c
pthread_join(tid, NULL);   // wait for tid; NULL = ignore what the thread returns
```

### Detach (don't wait, just let it run)
```c
pthread_detach(tid);
```

### Exit early from inside the thread
```c
pthread_exit(NULL);
```

### Passing arguments — the #1 gotcha
- A thread function only takes **one** `void *` argument.
- To pass an `int`, give it a pointer to an int (`&ids[i]`), and inside the
  thread cast it back: `int id = *(int*)arg;`.
- DON'T pass `&i` from a loop — every thread will share the same `i`.
  Use a per-thread slot, e.g. `int ids[5]; pthread_create(..., &ids[i]);`.

Example with easy names:
```c
int ids[3] = {1, 2, 3};

pthread_create(
    &threads[0],             // where to store thread 0's ID
    NULL,                    // default thread settings
    function_i_want_run,     // the function this thread runs
    &ids[0]                  // address of this thread's argument
);
```

Inside the thread:
```c
void *function_i_want_run(void *arg) {
    int id = *(int *)arg;    // go to the address, grab the int stored there
    return NULL;
}
```

---

## 2. Mutexes — `pthread_mutex_t`

A mutex = "lock". Only one thread can hold it at a time.

### Declare
```c
pthread_mutex_t m;
```

### Initialize (two ways)
```c
// (a) Static — at declaration, defaults only:
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

// (b) Dynamic — at runtime:
pthread_mutex_init(&m, NULL);   // &m = mutex to initialize; NULL = default settings
```

### Lock / Unlock
```c
pthread_mutex_lock(&m);     // blocks until the lock is free
// ... critical section ...
pthread_mutex_unlock(&m);
```

### Try without blocking
```c
if (pthread_mutex_trylock(&m) == 0) {
    // got the lock
    pthread_mutex_unlock(&m);
}
```

### Destroy when done
```c
pthread_mutex_destroy(&m);
```

### Rules of thumb
- **Always unlock** every path out of the critical section.
- Keep critical sections **short**.
- **Same lock order** in every thread to avoid deadlock.

---

## 3. Condition Variables — `pthread_cond_t`

A cond var = "I'll go to sleep until something interesting happens." Always
used **with a mutex**.

### Declare + init
```c
pthread_cond_t c;
pthread_cond_init(&c, NULL);   // &c = cond var to initialize; NULL = defaults
// or static: pthread_cond_t c = PTHREAD_COND_INITIALIZER;
```

### The classic pattern

Waiter:
```c
pthread_mutex_lock(&m);
while (!condition_is_true)         // ALWAYS use while, never if
    pthread_cond_wait(&c, &m);     // atomically: unlocks m, sleeps, relocks m
// ... do the work ...
pthread_mutex_unlock(&m);
```

Signaler:
```c
pthread_mutex_lock(&m);
make_condition_true();
pthread_cond_signal(&c);   // wake ONE waiter
// or pthread_cond_broadcast(&c);  // wake ALL waiters
pthread_mutex_unlock(&m);
```

### Destroy
```c
pthread_cond_destroy(&c);
```

### Rules
- Use `while`, not `if`, around `pthread_cond_wait` (spurious wakeups happen).
- You must hold the mutex when calling `pthread_cond_wait`.
- `pthread_cond_signal` / `broadcast` should also be done with the mutex held
  (textbook style — keeps things clean).

---

## 4. Barriers — `pthread_barrier_t`

A barrier = "everyone wait here until N threads have arrived, then all go."

### Declare + init
```c
pthread_barrier_t b;
pthread_barrier_init(&b, NULL, N);
// &b   = barrier to initialize
// NULL = default settings
// N    = how many threads must reach the barrier before anyone continues
```

### Use
```c
pthread_barrier_wait(&b);   // blocks until N threads have called this
```

### Destroy
```c
pthread_barrier_destroy(&b);
```

Great for "everyone get ready, THEN start at the same time" or
"finish phase 1 before any thread starts phase 2."

---

## 5. Read-Write Locks — `pthread_rwlock_t`

Many readers OR one writer (built-in version of the readers-writers problem).

### Declare + init
```c
pthread_rwlock_t rw;
pthread_rwlock_init(&rw, NULL);   // &rw = rwlock to initialize; NULL = defaults
```

### Lock for reading (shared)
```c
pthread_rwlock_rdlock(&rw);
// ... read ...
pthread_rwlock_unlock(&rw);
```

### Lock for writing (exclusive)
```c
pthread_rwlock_wrlock(&rw);
// ... write ...
pthread_rwlock_unlock(&rw);
```

### Destroy
```c
pthread_rwlock_destroy(&rw);
```

---

## 6. Semaphores — `sem_t` (bonus)

Different header: `#include <semaphore.h>`. Not in `pthread.h`.

```c
sem_t s;
sem_init(&s, 0, 1);
// &s = semaphore to initialize
// 0  = shared between threads in the same process
// 1  = starting value
sem_wait(&s);         // P() — decrement, block if 0
sem_post(&s);         // V() — increment, wake a waiter
sem_destroy(&s);
```

A semaphore with initial value 1 acts like a mutex.
A semaphore with initial value N can let up to N threads through.

---

## 7. Standard Init / Cleanup Skeleton

```c
pthread_mutex_t m;
pthread_cond_t  c;
pthread_barrier_t b;

int main(void) {
    pthread_mutex_init(&m, NULL);
    pthread_cond_init(&c, NULL);
    pthread_barrier_init(&b, NULL, NUM_THREADS);

    pthread_t threads[NUM_THREADS];
    int ids[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, worker, &ids[i]);
    }
    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], NULL);

    pthread_mutex_destroy(&m);
    pthread_cond_destroy(&c);
    pthread_barrier_destroy(&b);
    return 0;
}
```

---

## 8. Tiny Argument Reference In Easy English

Use these names in your head when reading the real function calls.

```c
pthread_create(
    &where_to_store_thread_id,
    NULL,
    function_i_want_thread_to_run,
    &argument_i_want_to_pass
);
```

- `&where_to_store_thread_id`: address of the `pthread_t` variable, like `&tid` or `&threads[i]`.
- `NULL`: default thread settings.
- `function_i_want_thread_to_run`: the function the new thread starts in.
- `&argument_i_want_to_pass`: address of the data the thread receives as `void *arg`.

```c
pthread_join(thread_i_am_waiting_for, NULL);
```

- `thread_i_am_waiting_for`: the `pthread_t` thread you want main to wait on.
- `NULL`: ignore the return value from the thread.

```c
pthread_mutex_init(&mutex_i_want_ready, NULL);
pthread_cond_init(&condition_variable_i_want_ready, NULL);
pthread_rwlock_init(&rwlock_i_want_ready, NULL);
```

- First argument: address of the thing you are initializing.
- `NULL`: default settings.

```c
pthread_barrier_init(&barrier_i_want_ready, NULL, how_many_threads_must_arrive);
```

- `&barrier_i_want_ready`: address of the barrier.
- `NULL`: default settings.
- `how_many_threads_must_arrive`: number of threads that must call `pthread_barrier_wait`.

```c
sem_init(&semaphore_i_want_ready, shared_between_processes, starting_value);
```

- `&semaphore_i_want_ready`: address of the semaphore.
- `shared_between_processes`: usually `0` for this class, meaning shared between threads.
- `starting_value`: how many threads can pass immediately before blocking starts.

---

## 9. Common Mistakes Checklist

- [ ] Forgot to `#include <pthread.h>` or to link with `-pthread`.
- [ ] Passed `&i` from a loop instead of `&ids[i]` (all threads see same value).
- [ ] Used `if` instead of `while` around `pthread_cond_wait`.
- [ ] Called `pthread_cond_wait` / `signal` without holding the mutex.
- [ ] Returned from `main` without `pthread_join` — threads get killed.
- [ ] Locked a mutex twice in the same thread (deadlock — unless recursive).
- [ ] Different threads lock A then B vs. B then A → deadlock.
- [ ] Forgot to `pthread_*_destroy` after the threads finished.
