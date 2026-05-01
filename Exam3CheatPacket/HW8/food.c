/* ============================================================================
 * food.c — Homework 8: A "Restaurant" Producer/Consumer Simulation
 * ============================================================================
 *
 * BIG PICTURE (read this first!):
 *
 *   Imagine a restaurant kitchen.
 *
 *     - CONSUMERS = customers placing orders.  Each customer wants a "meal"
 *       made of 3 different food categories: 0 (e.g. appetizer),
 *       1 (e.g. main course), and 2 (e.g. dessert).
 *
 *     - PRODUCERS = chefs in the kitchen who prepare individual food items.
 *
 *     - The LINKED LIST `p` is the "order ticket rail": every customer puts
 *       three tickets on it (one for each category 0, 1, 2).  Chefs grab
 *       tickets off the rail one by one and prepare that food item.
 *
 *     - The 2D BUFFER `q` is the "pickup counter".  It has 3 columns
 *       (one per food category).  Chefs place finished food into the right
 *       column.  A customer can only leave with a meal once column 0,
 *       column 1, AND column 2 each have at least one item ready.
 *
 *   Because many threads (chefs and customers) touch the SAME data at the
 *   same time, we must protect it with synchronization tools:
 *
 *     - pthread_mutex_t  -> a "lock" so only one thread edits data at once.
 *     - pthread_cond_t   -> a "waiting room" where threads sleep until
 *                           something they care about becomes true.
 *     - pthread_barrier_t-> a "rendezvous point" where threads wait for each
 *                           other before everybody continues.
 * ============================================================================
 */

#include <pthread.h>      // POSIX threads: pthread_create, mutexes, cond vars
#include <stdio.h>        // printf
#include <stdlib.h>       // malloc, free, atoi, exit
#include <unistd.h>       // usleep (sleep in microseconds)
#include <assert.h>       // assert(...) sanity checks
#include "linked-list.h"  // create_node, add_last, remove_first, etc.

/* MAX is the largest number of items that can sit in any one column of the
 * pickup counter at the same time.  The user passes a buffer "size" on the
 * command line, but it can never exceed MAX. */
#define MAX 10

/* ----------------------------------------------------------------------------
 * list_t — a thread-safe wrapper around a linked list of "order tickets".
 *
 * The linked-list itself is just nodes connected by `next` pointers (see
 * linked-list.h).  We bundle the head/tail pointers together with a mutex
 * so any thread that wants to add or remove a node can lock the mutex first
 * and avoid corrupting the list.
 * --------------------------------------------------------------------------*/
typedef struct {
    node    *head, *tail;     // first and last node of the linked list
    pthread_mutex_t mutex;    // lock that protects head/tail/next pointers
} list_t;

/* ----------------------------------------------------------------------------
 * two_d_buffer — the "pickup counter" with 3 columns.
 *
 *   buf[row][col]   col = 0, 1, or 2 (food category)
 *                   row = 0..size-1   (slot inside that column)
 *
 *   counts[col]     how many items are currently sitting in column `col`
 *                   (also serves as the index of the next free slot)
 *
 *   size            capacity of each column
 *   remain          how many more items still need to be produced in total
 *                   (used by main() to seed the value; not strictly needed
 *                    for correctness in this version)
 *
 *   mutex           protects ALL fields of this struct
 *   produce_cond    condition variable producers wait on when a column is FULL
 *   consume_cond    condition variable consumers wait on when ANY column is empty
 * --------------------------------------------------------------------------*/
typedef struct {
	int size;
        int buf[MAX][3];
        int remain;
        int counts[3];            //current indexes
        pthread_mutex_t mutex;
        pthread_cond_t produce_cond;
        pthread_cond_t consume_cond;
}two_d_buffer;

/* ============================================================================
 * add_to_buffer — called by a PRODUCER (chef) to drop a finished food item
 *                 into column `col` of the pickup counter.
 *
 * Classic "bounded buffer producer" pattern:
 *   1. Lock the mutex (only one thread touches the buffer at a time).
 *   2. If this column is already FULL, go to sleep on `produce_cond` and
 *      release the lock.  We use `while` (not `if`) because of "spurious
 *      wakeups" and because by the time we re-acquire the lock another
 *      producer may have already filled the column again.
 *   3. Once there is room, write the item into the next free slot
 *      (buf[counts[col]][col]) and bump counts[col].
 *   4. decrement `remain` (one fewer item left to produce overall).
 *   5. Wake up ALL consumers — adding to this column may have completed the
 *      "1-of-each-category" condition some consumer is waiting on.
 *   6. Unlock the mutex.
 * ============================================================================
 */
void add_to_buffer(int item, int col, two_d_buffer *p)
{
	//TODO
	//fill in code below
	pthread_mutex_lock(&p->mutex);
	while(p->counts[col] >= p->size)
	{
		// Column `col` is full — wait until a consumer empties a slot.
		// pthread_cond_wait atomically: (a) unlocks p->mutex,
		//                               (b) puts us to sleep,
		//                               (c) re-locks p->mutex on wake-up.
		pthread_cond_wait(&p->produce_cond, &p->mutex);
	}
	p->buf[p->counts[col]][col] = item;   // place item in the next free row
	p->counts[col]++;                     // one more item now in this column
	p->remain--;
	pthread_cond_broadcast(&p->consume_cond); // tell every consumer to recheck
	pthread_mutex_unlock(&p->mutex);
}

/* ============================================================================
 * remove_from_buffer — called by a CONSUMER (customer) to grab one complete
 *                      meal: one item from column 0, one from column 1, and
 *                      one from column 2.
 *
 * Classic "bounded buffer consumer" pattern, but with a twist:
 *   - We need ALL three columns to have at least one item before we can take
 *     a meal.  If ANY column is empty, we wait.
 *   - After taking the FIRST row of each column (buf[0][col]), we shift
 *     every remaining row down by one so that buf[0][col] is again the next
 *     item to take (a simple FIFO using an array).
 *   - Finally, broadcast on `produce_cond` because we just freed up one slot
 *     in every column — producers blocked on a full column might now proceed.
 * ============================================================================
 */
void remove_from_buffer(int *a, int *b, int *c, two_d_buffer *p)
{
	//TODO
	//fill in code below
	pthread_mutex_lock(&p->mutex);
	while(p->counts[0] == 0 || p->counts[1] == 0 || p->counts[2] == 0)
	{
		// At least one column is empty — we cannot form a complete meal yet.
		pthread_cond_wait(&p->consume_cond, &p->mutex);
	}
	// Take one item from each column (the oldest one, at row 0).
	*a = p->buf[0][0];
	*b = p->buf[0][1];
	*c = p->buf[0][2];

	// Shift each column down by one so buf[0][col] is the next item.
	for(int col = 0; col < 3; col++)
	{
		for(int i = 1; i < p->counts[col]; i++)
		{
			p->buf[i-1][col] = p->buf[i][col];
		}
		p->counts[col]--;
	}
	// We just freed a slot in every column — wake up any producer that was
	// waiting because its target column was full.
	pthread_cond_broadcast(&p->produce_cond);
	pthread_mutex_unlock(&p->mutex);
}

/* prepare() simulates a chef spending some time cooking.  Bigger item numbers
 * take a tiny bit longer.  usleep takes microseconds (1/1,000,000 sec). */
void prepare(int item)
{
	usleep((item + 1)*100);
}

/* ----------------------------------------------------------------------------
 * thread_data — every thread (consumer or producer) gets a pointer to one
 *               of these.  It tells the thread:
 *                 - its numeric id (for printing),
 *                 - which shared list `p` and shared buffer `q` to use,
 *                 - a counter to record how many items it ended up producing,
 *                 - the barrier all threads must hit before "real work" begins.
 * --------------------------------------------------------------------------*/
struct thread_data
{
	int id;
    	list_t *p;
    	two_d_buffer *q;
	int total;			//total items produced by a producer
	pthread_barrier_t *p_barrier;
};

/* ============================================================================
 * thread_consume — the function each CONSUMER thread runs.
 *
 * Sequence of events per consumer:
 *   1. Build three "order tickets" (nodes) for categories 0, 1, 2.
 *   2. Lock the list, append all three tickets to the end, unlock.
 *      (This is the "place my order on the rail.")
 *   3. Wait at the barrier so NO producer starts cooking until EVERY consumer
 *      has finished placing its 3 tickets.  This guarantees producers see a
 *      fully-loaded order list and never quit early thinking the list is empty.
 *   4. After the barrier opens, call remove_from_buffer to wait for one
 *      complete meal (one item from each of 3 columns) and print it.
 * ============================================================================
 */
void* thread_consume(void* threadarg)
{
    	struct thread_data* my_data = (struct thread_data*) threadarg;
	int id = my_data->id;
	list_t *p = my_data->p;

	// Build the three order tickets this customer wants prepared.
	node *n1 = create_node(0);     // ticket for category 0
	node *n2 = create_node(1);     // ticket for category 1
	node *n3 = create_node(2);     // ticket for category 2

	//TODO
	//fill in code below to add n1, n2, and n3 to the linked-list pointed by p
	// Critical section: only one thread at a time may touch the list.
	pthread_mutex_lock(&p->mutex);
	add_last(&p->head, &p->tail, n1);
	add_last(&p->head, &p->tail, n2);
	add_last(&p->head, &p->tail, n3);
	pthread_mutex_unlock(&p->mutex);

	// Rendezvous: wait here until ALL n_consumer + n_producer threads arrive.
	// Without this, a producer could start, find the list empty, and exit
	// before consumers had a chance to put their tickets in.
	pthread_barrier_t *p_barrier = my_data->p_barrier;
	pthread_barrier_wait(p_barrier);

	// Now wait for a complete 3-item meal to be ready and pick it up.
	two_d_buffer *q = my_data->q;
	int a, b, c;
	remove_from_buffer(&a, &b, &c, q);
	printf("consumer %04d (%d %d %d)\n", id, a, b, c);
	pthread_exit(NULL);
}

/* ============================================================================
 * thread_produce — the function each PRODUCER thread runs.
 *
 * Sequence of events per producer:
 *   1. Wait at the barrier (same one consumers use).  Producers cannot peek
 *      at the order list before consumers have added all their tickets.
 *   2. Loop:
 *        a. Lock the list and remove the first ticket.  Unlock.
 *        b. If the list was empty (remove_first returned NULL), we're done.
 *        c. Otherwise, "cook" the item (sleep a bit), then place it into the
 *           appropriate column of the 2D buffer.  Track how many we cooked.
 *
 * Note: the value `item` IS the column index (0, 1, or 2) because consumers
 * only ever insert tickets with values 0, 1, and 2.
 * ============================================================================
 */
void* thread_produce(void* threadarg)
{
	struct thread_data* my_data = (struct thread_data*) threadarg;
        list_t *p = my_data->p;
        pthread_barrier_t *p_barrier = my_data->p_barrier;
        // Wait until every consumer has finished placing tickets.
        pthread_barrier_wait(p_barrier);
	two_d_buffer *q = my_data->q;

	int done = 0;
	while(!done)
	{
		//TODO
		//fill in code below
		// Critical section: lock list, take one ticket, unlock.
		pthread_mutex_lock(&p->mutex);
		node *nd = remove_first(&p->head, &p->tail);
		pthread_mutex_unlock(&p->mutex);

		if(nd == NULL)
		{
			// No more tickets — this producer can go home.
			done = 1;
			break;
		}

		int item = nd->v;     // 0, 1, or 2 (which category to cook)
		free(nd);             // we copied the value, the node is no longer needed
		prepare(item);        // simulate cooking time
		add_to_buffer(item, item, q);  // drop finished food onto pickup counter
		my_data->total++;     // remember how many items I produced
	}

        pthread_exit(NULL);
}

/* ============================================================================
 * main — set up the shared structures, spawn threads, wait, then clean up.
 *
 * Command line:  ./food n_consumer n_producer buffer_size
 *
 *   n_consumer  : how many customer threads to create
 *   n_producer  : how many chef threads to create
 *   buffer_size : capacity of EACH column on the pickup counter (<= MAX)
 * ============================================================================
 */
int main(int argc, char *argv[])
{
	// -------- 1. Parse and sanity-check command line arguments. ----------
	if(argc < 4) {
		printf("Usage: %s n_consumer n_producer buffer_size\n", argv[0]);
		return -1;
	}
	int n_consumer = atoi(argv[1]);
	assert(n_consumer <= 3000);
	int n_producer = atoi(argv[2]);
	assert(n_producer <= 3000);
	int size = atoi(argv[3]);
	assert(size <= MAX);

	// -------- 2. Create the shared LINKED LIST (the order rail). ---------
	list_t *p = (list_t *)malloc(sizeof(list_t));
	if(p==NULL)
	{
		perror("Cannot allocate memeory.\n");
		return -1;
	}
	p->head = NULL;
	p->tail = NULL;
	pthread_mutex_init(&p->mutex, NULL);   // initialize the list's lock

	// -------- 3. Create the shared 2D BUFFER (the pickup counter). -------
	two_d_buffer *q = malloc(sizeof(two_d_buffer));
        q->size = size;
        // Each consumer asks for 3 items, so 3 * n_consumer items must be made.
        q->remain = 3*n_consumer;
        q->counts[0] = 0; q->counts[1] = 0; q->counts[2] = 0;
	pthread_mutex_init(&q->mutex, NULL);
    	pthread_cond_init (&q->produce_cond, NULL);
    	pthread_cond_init (&q->consume_cond, NULL);

	// -------- 4. Create the BARRIER all threads must hit. ----------------
	// The "trip count" is n_consumer + n_producer: every thread must call
	// pthread_barrier_wait exactly once before any of them passes through.
	pthread_barrier_t barrier;
	pthread_barrier_init(&barrier, NULL, n_consumer + n_producer);

	// Arrays for thread handles and per-thread argument structs.
    	pthread_t threads[n_consumer + n_producer];
    	struct thread_data thread_data_array[n_consumer + n_producer];
    	int rc, t;

	// -------- 5. Spawn CONSUMER threads. ---------------------------------
	for(t=0; t<n_consumer; t++ ) {
        	thread_data_array[t].id = t;
		thread_data_array[t].p = p;
		thread_data_array[t].q = q;
		thread_data_array[t].total = 0;
		thread_data_array[t].p_barrier = &barrier;

		//TODO
		//complete the following line of code
		// pthread_create arguments:
		//   &threads[t]            -> where to store the new thread's handle
		//   NULL                   -> default thread attributes
		//   thread_consume         -> function the new thread will run
		//   &thread_data_array[t]  -> argument passed to that function
		rc = pthread_create(&threads[t], NULL, thread_consume, &thread_data_array[t]);
        	if (rc) {
            		printf("ERROR; return code from pthread_create() is %d\n", rc);
            		exit(-1);
        	}
    	}

	// -------- 6. Spawn PRODUCER threads. ---------------------------------
	// Their slots in the arrays come AFTER all consumer slots.
        for(t=0; t<n_producer; t++ ) {
                thread_data_array[n_consumer + t].id = t;
                thread_data_array[n_consumer + t].p = p;
                thread_data_array[n_consumer + t].q = q;
		thread_data_array[n_consumer + t].total = 0;
		thread_data_array[n_consumer + t].p_barrier = &barrier;
		//TODO
		//complete the follow line of code
                rc = pthread_create(&threads[n_consumer + t], NULL, thread_produce, &thread_data_array[n_consumer + t]);
                if (rc) {
                        printf("ERROR; return code from pthread_create() is %d\n", rc);
                        exit(-1);
                }
        }

	// -------- 7. Wait for ALL threads to finish (join). ------------------
	// pthread_join blocks until the given thread has called pthread_exit
	// (or returned from its start function).  We must join every thread,
	// otherwise their resources leak.
    	for(t=0; t<n_consumer + n_producer; t++ )
    	{
        	rc = pthread_join( threads[t], NULL );
        	if( rc ){
            	printf("ERROR; return code from pthread_join() is %d\n", rc);
            	exit(-1);
        	}
    	}

	// -------- 8. Tally up how many items the producers cooked total. -----
	// Only producer slots have a meaningful `total`, so we skip the first
	// n_consumer entries and sum the rest.
	int total = 0;
	//TODO
	//fill in code below
	for(t = 0; t < n_producer; t++)
	{
		total += thread_data_array[n_consumer + t].total;
	}

	printf("total = %d\n", total);

	// -------- 9. Clean up: destroy locks/cond vars and free memory. ------
    	pthread_mutex_destroy(&p->mutex);
    	free(p);

	pthread_mutex_destroy(&q->mutex);
	pthread_cond_destroy(&q->consume_cond);
	pthread_cond_destroy(&q->produce_cond);
	free(q);

	pthread_barrier_destroy(&barrier);
    	return 0;
}
