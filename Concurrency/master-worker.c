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

int in = 0, out = 0, count = 0;
int produced_count = 0; // total number produced so far (0...M)
int consumed_count = 0; // total number consumed so far (0...M)
int M, N, C, P;         // M(total number to produce)
                        // N(buffer size)
                        // C(number of consumer)
                        // P(number of producer)
int *buffer;

/*
 * Producer must lock the mutex, wait while buffer is full (count == N)
 * Produce the next number
 * Insert into buffer
 * Signal consumers that buffer is not empty
 * Unlock mutex
 */
void *producer(void *arg) {
  int id = *(int *)arg;
  free(arg);
  while (1) {
    pthread_mutex_lock(&mutex);

    // Wait if buffer is full
    while (count == N) {
      pthread_cond_wait(&full, &mutex);
    }
    // if all numbers produced, exit
    if (produced_count >= M) {
      pthread_mutex_unlock(&mutex);
      break;
    }

    // produce next number
    int num = produced_count;
    produced_count++;

    // insert into buffer
    buffer[in] = num;
    in = (in + 1) % N;
    count++;

    printf("Producer %d produced %d\n", id, num);

    // Signal that buffer is not empty
    pthread_cond_signal(&empty);
    pthread_mutex_unlock(&mutex);
  }
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
  int id = *(int *)arg;
  while (1) {
    pthread_mutex_lock(&mutex);

    // Wait until buffer has data, but also check if done.
    while (count == 0 && consumed_count < M) {
      pthread_cond_wait(&empty, &mutex);
    }

    // if all number have been consumed, exit
    if (consumed_count >= M) {
      pthread_mutex_unlock(&mutex);
      break;
    }

    // consume the next number
    int num = buffer[out];
    out = (out + 1) % N;
    count--;
    consumed_count++;

    printf("Consumer %d consumed %d\n", id, num);

    // signal that buffer is not full
    pthread_cond_signal(&full);
    pthread_mutex_unlock(&mutex);
  }
  return NULL;
}
// TODO Update main function
// Create both producer and consumer threads (P producer, C consumer)
// Join all threads after they finish
// Free resources

int main(int argc, char *argv[]) {
  // Parse command-line arguments
  if (argc != 5) {
    fprintf(stderr,
            "Usage: %s, M (Total number) N (Maximum shared buffer size) C "
            "(Number of consumers) P (number of producers)\n",
            argv[0]);
    exit(EXIT_FAILURE);
  }
  M = atoi(argv[1]);
  N = atoi(argv[2]);
  C = atoi(argv[3]);
  P = atoi(argv[4]);

  if (M <= 0 || N <= 0 || C <= 0 || P <= 0) {
    fprintf(stderr, "All arguments must be positive integers.\n");
    exit(EXIT_FAILURE);
  }

  // Allocate buffer
  buffer = (int *)malloc(N * sizeof(int));
  if (buffer == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }

  // Create producer threads
  pthread_t producers[P];
  for (int i = 0; i < P; i++) {
    int *id = (int *)malloc(sizeof(int));
    *id = i;
    if (pthread_create(&producers[i], NULL, producer, id) != 0) {
      perror("pthread_create");
      exit(EXIT_FAILURE);
    }
  }

  // Create consumer threads
  pthread_t consumers[C];
  for (int i = 0; i < C; i++) {
    int *id = (int *)malloc(sizeof(int));
    *id = i;
    if (pthread_create(&consumers[i], NULL, consumer, id) != 0) {
      perror("pthread_create");
      exit(EXIT_FAILURE);
    }
  }

  // Wait for all producers to finish
  for (int i = 0; i < P; i++) {
    pthread_join(producers[i], NULL);
  }

  // Wait for all consumers to finish
  for (int i = 0; i < C; i++) {
    pthread_join(consumers[i], NULL);
  }

  free(buffer);
  printf("All producers finished. Exiting.\n");
  return 0;
}
