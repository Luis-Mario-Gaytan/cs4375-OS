/*
 * master-worker.c
 *
 * This program takes four command-line arguments:
 *  M - Total number of integers to produce (0 to M-1)
 *  N - Maximum size of the shared buffer
 *  C - number of workers threads
 *  P - number of master threads
 *
 *  P producer thread that generate integers 0 ... M-1
 *  C consumer (worker) threads that take numbers from a shared buffer size N
 * and print them. Each number must be produce exactly once and consumed exactly
 * once. Produces must block if buffer is full; consumer must block if buffer is
 * empty. Use only condition variables for waiting/signaling - no busy waiting.
 */
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <wait.h>

// Synchronization Primitives
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full =
    PTHREAD_COND_INITIALIZER; // Signaled when buffer has space
pthread_cond_t empty =
    PTHREAD_COND_INITIALIZER; // Signaled when buffer has data

int produced_count = 0; // total number produced so far (0...M)
int consumed_count = 0; // total number consumed so far (0...M)

int item_to_produce, curr_buf_size;
int total_items, max_buf_size, num_workers, num_masters;

int *buffer;

/*
 * Producer must lock the mutex, wait while buffer is full (count == N)
 * Produce the next number
 * Insert into buffer
 * Signal consumers that buffer is not empty
 * Unlock mutex
 */

void *producer(void *arg) {
  // TODO implement producer function.
  return NULL;
}

/*
 * Consumer must Lock mutex, wait for buffer empty (count == 0) and there are
 * still numbers to consume When woken check if there is data. If buffer is
 * empty but all numbers consumed, exit
 * Remove a number from buffer
 * Print the number
 * Signal producer that buffer is not full
 * Unlock mutex
 */
void *consumer(void *arg) {
  // TODO implement consumer function
  return NULL;
}
void print_produced(int num, int master) {

  printf("Produced %d by master %d\n", num, master);
}

void print_consumed(int num, int worker) {

  printf("Consumed %d by worker %d\n", num, worker);
}

// produce items and place in buffer
// modify code below to synchronize correctly
void *generate_requests_loop(void *data) {
  int thread_id = *((int *)data);

  while (1) {

    if (item_to_produce >= total_items) {
      break;
    }

    buffer[curr_buf_size++] = item_to_produce;
    print_produced(item_to_produce, thread_id);
    item_to_produce++;
  }
  return 0;
}

// write function to be run by worker threads
// ensure that the workers call the function print_consumed when they consume an
// item
// TODO Update main function
// Create both producer and consumer threads (P producer, C consumer)
// Join all threads after they finish
// Free resources

int main(int argc, char *argv[]) {
  int *master_thread_id;
  pthread_t *master_thread;
  item_to_produce = 0;
  curr_buf_size = 0;

  int i;

  if (argc < 5) {
    printf("./master-worker #total_items #max_buf_size #num_workers #masters "
           "e.g. ./exe 10000 1000 4 3\n");
    exit(1);
  } else {
    num_masters = atoi(argv[4]);
    num_workers = atoi(argv[3]);
    total_items = atoi(argv[1]);
    max_buf_size = atoi(argv[2]);
  }

  buffer = (int *)malloc(sizeof(int) * max_buf_size);

  // create master producer threads
  master_thread_id = (int *)malloc(sizeof(int) * num_masters);
  master_thread = (pthread_t *)malloc(sizeof(pthread_t) * num_masters);
  for (i = 0; i < num_masters; i++)
    master_thread_id[i] = i;

  for (i = 0; i < num_masters; i++)
    pthread_create(&master_thread[i], NULL, generate_requests_loop,
                   (void *)&master_thread_id[i]);

  // create worker consumer threads

  // wait for all threads to complete
  for (i = 0; i < num_masters; i++) {
    pthread_join(master_thread[i], NULL);
    printf("master %d joined\n", i);
  }

  /*----Deallocating Buffers---------------------*/
  free(buffer);
  free(master_thread_id);
  free(master_thread);

  return 0;
}
