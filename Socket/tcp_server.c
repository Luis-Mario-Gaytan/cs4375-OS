/*
 * TCP Multi-threaded File Server
 * CS4375 - Assignment 4
 *
 * This server handles multiple clients simultaneously using pthreads.
 * It serves two files: bio.txt and bio.pdf
 * Send data in chunks of 1200 bytes.
 */

#include <arpa/inet.h>
#include <bits/pthreadtypes.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define PORT 8080
#define CHUNK_SIZE 1200
#define MAX_CLIENT 100

/*Structure to pass client info to thread */
struct client_info {
  int client_socket;
  struct sockaddr_in client_addr;
  int client_id;
};

/* Mutex for thread-safe console output */
pthread_mutex_t print_mutext = PTHREAD_MUTEX_INITIALIZER;

/*
 * get_file_size - Returns the size of a file in bytes
 */

long get_file_size(const char *filename) {
  struct stat st;
  if (stat(filename, &st) == 0) {
    return st.st_size;
  }
  return -1;
}

