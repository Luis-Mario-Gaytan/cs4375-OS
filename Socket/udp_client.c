/*
 * UDP File Client
 * CS4375 - Assignment 4
 * 
 * Connects to the UDP server and downloads bio.txt and/or bio.pdf
 * Handles out-of-order packets and reassembles files.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <time.h>

#define PORT 8081
#define CHUNK_SIZE 1200
#define SERVER_IP "127.0.0.1"
#define TIMEOUT_SEC 5

/* Packet structure matching server */
struct packet {
    uint32_t seq_num;
    uint32_t total_chunks;
    uint32_t data_len;
    uint32_t file_type;
    char data[CHUNK_SIZE];
};

/* Structure to hold file being received */
struct file_buffer {
    char **chunks;
    uint32_t *chunk_sizes;
    uint32_t total_chunks;
    uint32_t received_chunks;
    uint32_t file_type;
    long file_size;
};

/*
 * init_file_buffer - Initialize file buffer structure
 */
void init_file_buffer(struct file_buffer *fb, uint32_t total_chunks, uint32_t file_type, long file_size) {
    fb->total_chunks = total_chunks;
    fb->received_chunks = 0;
    fb->file_type = file_type;
    fb->file_size = file_size;

    fb->chunks = malloc(total_chunks * sizeof(char *));
    fb->chunk_sizes = malloc(total_chunks * sizeof(uint32_t));

    for (uint32_t i = 0; i < total_chunks; i++) {
        fb->chunks[i] = malloc(CHUNK_SIZE);
        fb->chunk_sizes[i] = 0;
    }
}

/*
 * free_file_buffer - Free file buffer memory
 */
void free_file_buffer(struct file_buffer *fb) {
    if (fb->chunks) {
        for (uint32_t i = 0; i < fb->total_chunks; i++) {
            free(fb->chunks[i]);
        }
        free(fb->chunks);
    }
    free(fb->chunk_sizes);
}

/*
 * save_file - Save received chunks to file
 */
int save_file(struct file_buffer *fb, const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        perror("Error creating file");
        return -1;
    }

    long written = 0;
    for (uint32_t i = 0; i < fb->total_chunks; i++) {
        if (fb->chunk_sizes[i] > 0) {
            fwrite(fb->chunks[i], 1, fb->chunk_sizes[i], fp);
            written += fb->chunk_sizes[i];
        }
    }

    fclose(fp);
    printf("[CLIENT] Saved '%s' (%ld bytes, %u/%u chunks)\n", 
           filename, written, fb->received_chunks, fb->total_chunks);
    return 0;
}

int main(int argc, char *argv[]) {
    int client_id = 0;
    int request = 3;  /* Default: request both files */

    /* Parse command line arguments */
    if (argc >= 2) {
        client_id = atoi(argv[1]);
    }
    if (argc >= 3) {
        request = atoi(argv[2]);
    }

    int udp_socket;
    struct sockaddr_in serv_addr;
    socklen_t addr_len = sizeof(serv_addr);

    printf("  UDP File Client - ID: %d\n", client_id);

    /* Create UDP socket */
    if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }

    /* Set socket timeout for receiving */
    struct timeval tv;
    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = 0;
    setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    /* Configure server address */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        printf("Invalid address: %s\n", SERVER_IP);
        return -1;
    }

    printf("[CLIENT %d] Sending request to %s:%d...\n", client_id, SERVER_IP, PORT);

    /* Send request to server */
    int net_request = htonl(request);
    sendto(udp_socket, &net_request, sizeof(net_request), 0,
           (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    printf("[CLIENT %d] Waiting for files...\n", client_id);

    /* Receive files */
    int files_received = 0;
    struct file_buffer current_file;
    int receiving_file = 0;
    int done = 0;

    while (!done) {
        struct packet pkt;
        memset(&pkt, 0, sizeof(pkt));

        ssize_t recv_len = recvfrom(udp_socket, &pkt, sizeof(pkt), 0,
                                    (struct sockaddr *)&serv_addr, &addr_len);

        if (recv_len < 0) {
            if (receiving_file) {
                printf("[CLIENT %d] Timeout waiting for packets\n", client_id);
                /* Try to save what we have */
                if (current_file.received_chunks > 0) {
                    char filename[256];
                    if (current_file.file_type == 2) {
                        snprintf(filename, sizeof(filename), 
                                "received_bio_%d.pdf", client_id);
                    } else {
                        snprintf(filename, sizeof(filename), 
                                "received_bio_%d.txt", client_id);
                    }
                    save_file(&current_file, filename);
                    free_file_buffer(&current_file);
                }
            }
            break;
        }

        uint32_t seq_num = ntohl(pkt.seq_num);
        uint32_t total_chunks = ntohl(pkt.total_chunks);
        uint32_t data_len = ntohl(pkt.data_len);
        uint32_t file_type = ntohl(pkt.file_type);

        /* Check for completion marker */
        if (seq_num == 0xFFFFFFFF || file_type == 999) {
            printf("[CLIENT %d] All transfers complete!\n", client_id);
            done = 1;
            break;
        }

        /* Check for info packet (seq_num = 0) */
        if (seq_num == 0) {
            /* New file starting */
            if (receiving_file) {
                /* Save previous file if any */
                char filename[256];
                if (current_file.file_type == 2) {
                    snprintf(filename, sizeof(filename), 
                            "received_bio_%d.pdf", client_id);
                } else {
                    snprintf(filename, sizeof(filename), 
                            "received_bio_%d.txt", client_id);
                }
                save_file(&current_file, filename);
                free_file_buffer(&current_file);
                files_received++;
            }

            /* Initialize new file buffer */
            init_file_buffer(&current_file, total_chunks, file_type, data_len);
            receiving_file = 1;

            printf("[CLIENT %d] Starting new file: %s (%u chunks, %ld bytes)\n", 
                   client_id, pkt.data, total_chunks, (long)data_len);
            continue;
        }

        /* Regular data packet */
        if (receiving_file && seq_num > 0 && seq_num <= current_file.total_chunks) {
            uint32_t idx = seq_num - 1;
            if (current_file.chunk_sizes[idx] == 0) {
                memcpy(current_file.chunks[idx], pkt.data, data_len);
                current_file.chunk_sizes[idx] = data_len;
                current_file.received_chunks++;
            }
        }
    }

    /* Save last file if any */
    if (receiving_file) {
        char filename[256];
        if (current_file.file_type == 2) {
            snprintf(filename, sizeof(filename), 
                    "received_bio_%d.pdf", client_id);
        } else {
            snprintf(filename, sizeof(filename), 
                    "received_bio_%d.txt", client_id);
        }
        save_file(&current_file, filename);
        free_file_buffer(&current_file);
        files_received++;
    }

    printf("[CLIENT %d] Total files received: %d\n", client_id, files_received);
    printf("[CLIENT %d] Disconnecting...\n", client_id);

    close(udp_socket);
    return 0;
}
