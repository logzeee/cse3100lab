/* pc_multi.c
 * MANY producers, MANY consumers sharing one circular buffer.
 * Adds clean shutdown via a shared `items_left` counter.
 *
 * Rules added vs pc_circular.c:
 *   - producer stops when items_left == 0
 *   - consumer stops when items_left == 0 AND buffer is empty
 *   - broadcast on the way out so blocked threads wake up to exit
 *
 * gcc -Wall pc_multi.c -o pc_multi -lpthread
 */

#include <pthread.h>
#include <stdio.h>

#define SIZE 3
#define NP   2
#define NC   3
#define WORK 12          // total items across all producers

int buf[SIZE];
int in = 0, out = 0, count = 0;
int items_left = WORK;
int next_id    = 1;      // monotonic item id

pthread_mutex_t m;
pthread_cond_t  cP;
pthread_cond_t  cC;

void *producer(void *arg) {
    int id = *(int *)arg;
    while (1) {
        pthread_mutex_lock(&m);
        while (count == SIZE && items_left > 0)   // full but more to do
            pthread_cond_wait(&cP, &m);
        if (items_left == 0) {                    // nothing left to claim
            pthread_cond_broadcast(&cC);          // wake any sleeping consumer
            pthread_mutex_unlock(&m);
            return 0;
        }

        int x = next_id++;
        items_left--;
        buf[in] = x; in = (in + 1) % SIZE; count++;
        printf("[P%d] %d (count=%d, left=%d)\n", id, x, count, items_left);

        pthread_cond_signal(&cC);
        pthread_mutex_unlock(&m);
    }
}

void *consumer(void *arg) {
    int id = *(int *)arg;
    while (1) {
        pthread_mutex_lock(&m);
        while (count == 0 && items_left > 0)      // empty but more coming
            pthread_cond_wait(&cC, &m);
        if (count == 0 && items_left == 0) {      // empty AND done
            pthread_mutex_unlock(&m);
            return 0;
        }

        int x = buf[out]; out = (out + 1) % SIZE; count--;
        printf("        [C%d] got %d (count=%d)\n", id, x, count);

        pthread_cond_signal(&cP);
        pthread_mutex_unlock(&m);
    }
}

int main(void) {
    pthread_mutex_init(&m,  NULL);
    pthread_cond_init(&cP, NULL);
    pthread_cond_init(&cC, NULL);

    pthread_t P[NP], C[NC];
    int pid[NP], cid[NC];

    for (int i = 0; i < NP; i++) { pid[i] = i + 1;
        pthread_create(&P[i], 0, producer, &pid[i]); }
    for (int i = 0; i < NC; i++) { cid[i] = i + 1;
        pthread_create(&C[i], 0, consumer, &cid[i]); }

    for (int i = 0; i < NP; i++) pthread_join(P[i], 0);
    for (int i = 0; i < NC; i++) pthread_join(C[i], 0);

    pthread_mutex_destroy(&m);
    pthread_cond_destroy(&cP);
    pthread_cond_destroy(&cC);
    return 0;
}
