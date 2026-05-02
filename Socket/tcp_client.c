/*
 * TCP File Client 2 - With retry logic and better error handling
 * CS4375 - Assignment 4
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>

#define PORT 8080
#define CHUNK_SIZE 1200
#define SERVER_IP "127.0.0.1"
#define MAX_RETRIES 10
#define RETRY_DELAY_US 500000  /* 0.5 seconds */

int receive_file(int socket, const char *output_filename, long expected_size) {
    FILE *fp = fopen(output_filename, "wb");
    if (fp == NULL) {
        perror("Error creating output file");
        return -1;
    }

    char buffer[CHUNK_SIZE];
    long total_received = 0;
    int chunk_count = 0;

    while (total_received < expected_size) {
        size_t to_receive = CHUNK_SIZE;
        if (expected_size - total_received < CHUNK_SIZE) {
            to_receive = expected_size - total_received;
        }

        ssize_t received = recv(socket, buffer, to_receive, 0);
        if (received <= 0) {
            if (received < 0) perror("Error receiving data");
            break;
        }

        size_t written = fwrite(buffer, 1, received, fp);
        if (written != (size_t)received) {
            perror("Error writing to file");
            fclose(fp);
            return -1;
        }
        total_received += received;
        chunk_count++;
    }

    fclose(fp);
    printf("[CLIENT] Received '%s' (%ld bytes, %d chunks)\n", 
           output_filename, total_received, chunk_count);

    return (total_received == expected_size) ? 0 : -1;
}

int connect_with_retry(int *client_fd, struct sockaddr_in *serv_addr) {
    int retries = 0;

    while (retries < MAX_RETRIES) {
        *client_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (*client_fd < 0) {
            perror("Socket creation failed");
            return -1;
        }

        if (connect(*client_fd, (struct sockaddr *)serv_addr, sizeof(*serv_addr)) == 0) {
            return 0;  /* Success */
        }

        /* Connection failed, close and retry */
        close(*client_fd);
        retries++;

        if (retries < MAX_RETRIES) {
            printf("[CLIENT] Connection failed, retrying in %d ms... (%d/%d)\n",
                   RETRY_DELAY_US / 1000, retries, MAX_RETRIES);
            usleep(RETRY_DELAY_US);
        }
    }

    printf("[CLIENT] Failed to connect after %d retries\n", MAX_RETRIES);
    return -1;
}

int main(int argc, char *argv[]) {
    int client_id = 0;
    int request = 3;

    if (argc >= 2) {
        client_id = atoi(argv[1]);
    }
    if (argc >= 3) {
        request = atoi(argv[2]);
    }

    int client_fd;
    struct sockaddr_in serv_addr;

    printf("  TCP File Client - ID: %d\n", client_id);

    /* Configure server address */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        printf("Invalid address: %s\n", SERVER_IP);
        return -1;
    }

    /* Connect with retry logic */
    printf("[CLIENT %d] Connecting to %s:%d...\n", client_id, SERVER_IP, PORT);
    if (connect_with_retry(&client_fd, &serv_addr) < 0) {
        return -1;
    }
    printf("[CLIENT %d] Connected to server!\n", client_id);

    /* Send request to server */
    int net_request = htonl(request);
    if (send(client_fd, &net_request, sizeof(net_request), 0) < 0) {
        perror("Send failed");
        close(client_fd);
        return -1;
    }
    printf("[CLIENT %d] Requested option: %d\n", client_id, request);

    /* Receive files */
    int files_received = 0;

    while (1) {
        /* Receive file size */
        long net_file_size;
        ssize_t recv_len = recv(client_fd, &net_file_size, sizeof(net_file_size), 0);
        if (recv_len <= 0) {
            if (recv_len < 0) perror("Error receiving file size");
            printf("[CLIENT %d] Server closed connection\n", client_id);
            break;
        }

        long file_size = ntohl((uint32_t)net_file_size);

        /* Check for completion marker */
        if (file_size == 999) {
            printf("[CLIENT %d] All transfers complete!\n", client_id);
            break;
        }

        /* Receive file type */
        int net_type;
        recv_len = recv(client_fd, &net_type, sizeof(net_type), 0);
        if (recv_len <= 0) {
            perror("Error receiving file type");
            break;
        }
        int file_type = ntohl(net_type);

        /* Generate output filename */
        char output_filename[256];
        if (file_type == 2) {
            snprintf(output_filename, sizeof(output_filename), 
                     "received_bio_%d.pdf", client_id);
        } else {
            snprintf(output_filename, sizeof(output_filename), 
                     "received_bio_%d.txt", client_id);
        }

        printf("[CLIENT %d] Receiving file (%ld bytes)...\n", client_id, file_size);

        if (receive_file(client_fd, output_filename, file_size) == 0) {
            files_received++;
        }
    }

    printf("[CLIENT %d] Total files received: %d\n", client_id, files_received);
    printf("[CLIENT %d] Disconnecting...\n", client_id);

    close(client_fd);
    return 0;
}
