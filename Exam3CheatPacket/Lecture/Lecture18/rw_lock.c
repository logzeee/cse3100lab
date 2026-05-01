#include "rw_lock.h"

void rwlock_init(rwlock_t *lock) {
  pthread_mutex_init(&lock->mutex, NULL);
  pthread_cond_init(&lock->readers_ok, NULL);
  pthread_cond_init(&lock->writers_ok, NULL);
  lock->readers = 0;
  lock->writers = 0;
  lock->waiting_writers = 0;
}

void rwlock_acquire_read(rwlock_t *lock) {
  pthread_mutex_lock(&lock->mutex);
  while (lock->writers > 0 || lock->waiting_writers > 0) {
    pthread_cond_wait(&lock->readers_ok, &lock->mutex);
  }
  lock->readers++;
  pthread_mutex_unlock(&lock->mutex);
}

void rwlock_release_read(rwlock_t *lock) {
  pthread_mutex_lock(&lock->mutex);
  lock->readers--;
  if (lock->readers == 0 && lock->waiting_writers > 0) {
    pthread_cond_signal(&lock->writers_ok);
  }
  pthread_mutex_unlock(&lock->mutex);
}

void rwlock_acquire_write(rwlock_t *lock) {
  pthread_mutex_lock(&lock->mutex);
  lock->waiting_writers++;
  while (lock->readers > 0 || lock->writers > 0) {
    pthread_cond_wait(&lock->writers_ok, &lock->mutex);
  }
  lock->waiting_writers--;
  lock->writers = 1;
  pthread_mutex_unlock(&lock->mutex);
}

void rwlock_release_write(rwlock_t *lock) {
  pthread_mutex_lock(&lock->mutex);
  lock->writers = 0;
  if (lock->waiting_writers > 0) {
    pthread_cond_signal(&lock->writers_ok);
  } else {
    pthread_cond_broadcast(&lock->readers_ok);
  }
  pthread_mutex_unlock(&lock->mutex);
}
