#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int shared_data = 0;
int reader_count = 0;          // how many readers are currently reading

pthread_mutex_t mutex;         // protects reader_count
pthread_mutex_t write_lock;    // exclusive access for writers

void *reader(void *arg) {
    int id = *(int *)arg;

    while (1) {
        // --- Entry section ---
        pthread_mutex_lock(&mutex);
        reader_count++;
        if (reader_count == 1) {
            // First reader locks out writers
            pthread_mutex_lock(&write_lock);
        }
        pthread_mutex_unlock(&mutex);

        // --- Reading section ---
        printf("Reader %d reading: %d\n", id, shared_data);
        sleep(1);

        // --- Exit section ---
        pthread_mutex_lock(&mutex);
        reader_count--;
        if (reader_count == 0) {
            // Last reader lets writers in
            pthread_mutex_unlock(&write_lock);
        }
        pthread_mutex_unlock(&mutex);

        sleep(1);
    }
}

void *writer(void *arg) {
    int id = *(int *)arg;

    while (1) {
        // --- Entry section ---
        pthread_mutex_lock(&write_lock);  // blocks if anyone is reading/writing

        // --- Writing section ---
        shared_data++;
        printf("Writer %d wrote: %d\n", id, shared_data);
        sleep(1);

        // --- Exit section ---
        pthread_mutex_unlock(&write_lock);

        sleep(2);
    }
}

int main() {
    pthread_t r[3], w[2];
    int ids[] = {1, 2, 3, 4, 5};

    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&write_lock, NULL);

    // spawn 3 readers and 2 writers
    for (int i = 0; i < 3; i++)
        pthread_create(&r[i], NULL, reader, &ids[i]);
    for (int i = 0; i < 2; i++)
        pthread_create(&w[i], NULL, writer, &ids[i]);

    for (int i = 0; i < 3; i++) pthread_join(r[i], NULL);
    for (int i = 0; i < 2; i++) pthread_join(w[i], NULL);

    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&write_lock);

    return 0;
}
