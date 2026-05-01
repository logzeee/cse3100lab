#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

pthread_mutex_t lock1;
pthread_mutex_t lock2;

void *thread_func1(void *arg) {
  printf("Thread 1: Trying to acquire lock1\n");
  pthread_mutex_lock(&lock1);
  printf("Thread 1: Acquired lock1\n");
  sleep(1); // Simulate some work

  printf("Thread 1: Trying to acquire lock2\n");
  pthread_mutex_lock(&lock2);
  printf("Thread 1: Acquired lock2\n");

  // Critical section
  printf("Thread 1: In critical section\n");

  // Release locks
  pthread_mutex_unlock(&lock2);
  pthread_mutex_unlock(&lock1);

  return NULL;
}

void *thread_func2(void *arg) {
  printf("Thread 2: Trying to acquire lock2\n");
  pthread_mutex_lock(&lock2);
  printf("Thread 2: Acquired lock2\n");
  sleep(1); // Simulate some work

  printf("Thread 2: Trying to acquire lock1\n");
  pthread_mutex_lock(&lock1);
  printf("Thread 2: Acquired lock1\n");

  // Critical section
  printf("Thread 2: In critical section\n");

  // Release locks
  pthread_mutex_unlock(&lock1);
  pthread_mutex_unlock(&lock2);

  return NULL;
}

int main() {
  pthread_t t1, t2;

  // Initialize the mutexes
  pthread_mutex_init(&lock1, NULL);
  pthread_mutex_init(&lock2, NULL);

  // Create two threads
  pthread_create(&t1, NULL, thread_func1, NULL);
  pthread_create(&t2, NULL, thread_func2, NULL);

  // Wait for threads to finish
  pthread_join(t1, NULL);
  pthread_join(t2, NULL);

  // Destroy the mutexes
  pthread_mutex_destroy(&lock1);
  pthread_mutex_destroy(&lock2);

  return 0;
}
