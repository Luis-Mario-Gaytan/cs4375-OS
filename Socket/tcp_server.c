/*
 * TCP Multi-threaded File Server
 * CS4375 - Assignment 4
 * Serves bio.txt and bio.pdf to multiple clients simultaneously
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>

#define PORT 8080
#define CHUNK_SIZE 1200

struct client_info {
    int client_socket;
    struct sockaddr_in client_addr;
    int client_id;
};

pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

long get_file_size(const char *filename) {
    struct stat st;
    if (stat(filename, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

int send_file(int client_socket, const char *filename, const char *file_type) {
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("Error opening file");
        return -1;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    /* Send file size */
    long net_file_size = htonl((uint32_t)file_size);
    if (send(client_socket, &net_file_size, sizeof(net_file_size), 0) < 0) {
        perror("Error sending file size");
        fclose(fp);
        return -1;
    }

    /* Send file type: 1=txt, 2=pdf */
    int type = (strcmp(file_type, "pdf") == 0) ? 2 : 1;
    int net_type = htonl(type);
    if (send(client_socket, &net_type, sizeof(net_type), 0) < 0) {
        perror("Error sending file type");
        fclose(fp);
        return -1;
    }

    /* Send file data in chunks */
    char buffer[CHUNK_SIZE];
    size_t bytes_read;
    long total_sent = 0;
    int chunk_count = 0;

    while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, fp)) > 0) {
        ssize_t sent = send(client_socket, buffer, bytes_read, 0);
        if (sent < 0) {
            perror("Error sending data chunk");
            fclose(fp);
            return -1;
        }
        total_sent += sent;
        chunk_count++;
    }

    fclose(fp);

    pthread_mutex_lock(&print_mutex);
    printf("[SERVER] Sent '%s' (%ld bytes, %d chunks) to client\n", 
           filename, total_sent, chunk_count);
    pthread_mutex_unlock(&print_mutex);

    return 0;
}

void *handle_client(void *arg) {
    struct client_info *info = (struct client_info *)arg;
    int client_socket = info->client_socket;
    int client_id = info->client_id;

    pthread_mutex_lock(&print_mutex);
    printf("[SERVER] Client %d connected from %s:%d\n", 
           client_id, 
           inet_ntoa(info->client_addr.sin_addr),
           ntohs(info->client_addr.sin_port));
    pthread_mutex_unlock(&print_mutex);

    /* Receive client's file request */
    int request;
    ssize_t recv_len = recv(client_socket, &request, sizeof(request), 0);
    if (recv_len <= 0) {
        pthread_mutex_lock(&print_mutex);
        printf("[SERVER] Client %d disconnected or error\n", client_id);
        pthread_mutex_unlock(&print_mutex);
        close(client_socket);
        free(info);
        pthread_exit(NULL);
    }

    request = ntohl(request);

    pthread_mutex_lock(&print_mutex);
    printf("[SERVER] Client %d requested option: %d\n", client_id, request);
    pthread_mutex_unlock(&print_mutex);

    /* Send files based on request: 1=txt, 2=pdf, 3=both */
    if (request == 1 || request == 3) {
        pthread_mutex_lock(&print_mutex);
        printf("[SERVER] Sending bio.txt to client %d...\n", client_id);
        pthread_mutex_unlock(&print_mutex);

        send_file(client_socket, "bio.txt", "txt");
    }

    if (request == 2 || request == 3) {
        pthread_mutex_lock(&print_mutex);
        printf("[SERVER] Sending bio.pdf to client %d...\n", client_id);
        pthread_mutex_unlock(&print_mutex);

        send_file(client_socket, "bio.pdf", "pdf");
    }

    /* Send completion marker */
    int done = htonl(999);
    send(client_socket, &done, sizeof(done), 0);

    pthread_mutex_lock(&print_mutex);
    printf("[SERVER] Client %d transfer complete. Connection closed.\n", client_id);
    pthread_mutex_unlock(&print_mutex);

    close(client_socket);
    free(info);
    pthread_exit(NULL);
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int client_counter = 0;

    /* Create socket */
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    /* Allow socket reuse */
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                   &opt, sizeof(opt))) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    /* Configure address */
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    /* Bind socket */
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    /* Listen for connections */
    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("  TCP Multi-threaded File Server\n");
    printf("  Port: %d\n", PORT);
    printf("  Chunk Size: %d bytes\n", CHUNK_SIZE);
    printf("[SERVER] Waiting for connections...\n\n");

    /* Main server loop */
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int new_socket;

        new_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }

        client_counter++;

        /* Allocate memory for client info */
        struct client_info *info = malloc(sizeof(struct client_info));
        if (info == NULL) {
            perror("Malloc failed");
            close(new_socket);
            continue;
        }

        info->client_socket = new_socket;
        info->client_addr = client_addr;
        info->client_id = client_counter;

        /* Create thread to handle client */
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client, (void *)info) != 0) {
            perror("Thread creation failed");
            free(info);
            close(new_socket);
            continue;
        }

        /* Detach thread so it cleans up automatically */
        pthread_detach(thread_id);
    }

    close(server_fd);
    pthread_mutex_destroy(&print_mutex);
    return 0;
}
