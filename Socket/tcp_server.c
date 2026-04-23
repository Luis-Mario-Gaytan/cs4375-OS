#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <threads.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1200
#define FILE1 "bio.txt"
#define FILE2 "bio.pdf"

// Strucuture to pass client socket to thread
typedef struct {
  int client_fd;
  int client_id;
} client_info_t;

// Function to send a file over a socket
void send_file(int client_fd, const char *filename) {
  FILE *file = fopen(filename, "rb");
  if (!file) {
    perror("fopen");
    char error_msg[] = "ERROR: File not found\n";
    send(client_fd, error_msg, strlen(error_msg), 0);
    return;
  }

  char buffer[BUFFER_SIZE];
  size_t bytes_read;
  while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
    ssize_t sent = send(client_fd, buffer, bytes_read, 0);
    if (sent != bytes_read) {
      perror("send");
      break;
    }
  }
  fclose(file);
}

// Thread function to handle one client
void *handle_client(void *arg) {
  client_info_t *info = (client_info_t *)arg;
  int client_fd = info->client_fd;
  int client_id = info->client_id;
  free(info); // free the allocated strurcture
  printf("Handling client %d\n", client_id);
  // Send file1
  send_file(client_fd, FILE1);
  // Send file2
  send_file(client_fd, FILE2);
  shutdown(client_fd, SHUT_WR); // Indicates no more data
  close(client_fd);
  printf("Client %d finished\n", client_id);
  return NULL;
}

int main() {

  int server_fd, client_fd;
  struct sockaddr_in address;
  int addrlen = sizeof(address);
  int client_counter = 0;

  // Create socket
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }
}
