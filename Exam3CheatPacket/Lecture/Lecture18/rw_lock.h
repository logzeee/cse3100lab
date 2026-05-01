#ifndef READ_WRITE_LOCK_H
#define READ_WRITE_LOCK_H

#include <pthread.h>

typedef struct {
  pthread_mutex_t mutex;
  pthread_cond_t readers_ok;
  pthread_cond_t writers_ok;
  int readers;
  int writers;
  int waiting_writers;
} rwlock_t;

void rwlock_init(rwlock_t *lock);
void rwlock_acquire_read(rwlock_t *lock);
void rwlock_release_read(rwlock_t *lock);
void rwlock_acquire_write(rwlock_t *lock);
void rwlock_release_write(rwlock_t *lock);

#endif
