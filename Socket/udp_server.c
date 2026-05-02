/*
 * UDP Multi-threaded File Server
 * CS4375 - Assignment 4
 * 
 * This server handles multiple clients using UDP sockets and pthreads.
 * Since UDP is connectionless, we use a request-response model.
 * Sends data in chunks of 1200 bytes with sequence numbers for reliability.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <errno.h>

#define PORT 8081
#define CHUNK_SIZE 1200
#define MAX_CLIENTS 100
#define TIMEOUT_SEC 2

/* Packet structure for reliable UDP */
struct packet {
    uint32_t seq_num;       /* Sequence number */
    uint32_t total_chunks;  /* Total number of chunks */
    uint32_t data_len;      /* Length of data in this packet */
    uint32_t file_type;     /* 1 = txt, 2 = pdf */
    char data[CHUNK_SIZE];  /* Actual data */
};

/* Client request structure */
struct client_request {
    struct sockaddr_in client_addr;
    socklen_t addr_len;
    int request_type;
    int client_id;
};

pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;

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

/*
 * send_file_udp - Sends a file to the client using UDP with sequence numbers
 */
int send_file_udp(int udp_socket, struct sockaddr_in *client_addr, 
                  socklen_t addr_len, const char *filename, int file_type) {
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("Error opening file");
        return -1;
    }

    /* Get file size */
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    /* Calculate total chunks */
    uint32_t total_chunks = (file_size + CHUNK_SIZE - 1) / CHUNK_SIZE;
    if (total_chunks == 0) total_chunks = 1;

    pthread_mutex_lock(&print_mutex);
    printf("[SERVER] Sending '%s' (%ld bytes, %u chunks) via UDP\n", 
           filename, file_size, total_chunks);
    pthread_mutex_unlock(&print_mutex);

    /* Send file info packet first */
    struct packet info_pkt;
    memset(&info_pkt, 0, sizeof(info_pkt));
    info_pkt.seq_num = htonl(0);
    info_pkt.total_chunks = htonl(total_chunks);
    info_pkt.data_len = htonl((uint32_t)file_size);
    info_pkt.file_type = htonl(file_type);
    strcpy(info_pkt.data, filename);

    sendto(udp_socket, &info_pkt, sizeof(info_pkt), 0,
           (struct sockaddr *)client_addr, addr_len);

    /* Small delay to let client prepare */
    usleep(10000);

    /* Send file data in chunks */
    struct pkt_data {
        uint32_t seq_num;
        uint32_t total_chunks;
        uint32_t data_len;
        uint32_t file_type;
        char data[CHUNK_SIZE];
    };

    char buffer[CHUNK_SIZE];
    uint32_t seq_num = 1;
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, fp)) > 0) {
        struct pkt_data pkt;
        memset(&pkt, 0, sizeof(pkt));
        pkt.seq_num = htonl(seq_num);
        pkt.total_chunks = htonl(total_chunks);
        pkt.data_len = htonl((uint32_t)bytes_read);
        pkt.file_type = htonl(file_type);
        memcpy(pkt.data, buffer, bytes_read);

        ssize_t sent = sendto(udp_socket, &pkt, sizeof(pkt), 0,
                              (struct sockaddr *)client_addr, addr_len);

        if (sent < 0) {
            perror("Error sending UDP packet");
            fclose(fp);
            return -1;
        }

        seq_num++;

        /* Small delay to prevent network flooding */
        usleep(1000);
    }

    fclose(fp);

    pthread_mutex_lock(&print_mutex);
    printf("[SERVER] Finished sending '%s' (%u chunks)\n", filename, total_chunks);
    pthread_mutex_unlock(&print_mutex);

    return 0;
}

/*
 * handle_client_udp - Thread function to handle a UDP client request
 */
void *handle_client_udp(void *arg) {
    struct client_request *req = (struct client_request *)arg;
    int udp_socket;

    /* Create a new socket for this client communication */
    if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed in thread");
        free(req);
        pthread_exit(NULL);
    }

    /* Bind to any available port */
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = 0;  /* Let system choose port */

    if (bind(udp_socket, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        perror("Bind failed in thread");
        close(udp_socket);
        free(req);
        pthread_exit(NULL);
    }

    pthread_mutex_lock(&print_mutex);
    printf("[SERVER] Handling UDP client %d request: %d\n", 
           req->client_id, req->request_type);
    pthread_mutex_unlock(&print_mutex);

    /* Send requested files */
    if (req->request_type == 1 || req->request_type == 3) {
        send_file_udp(udp_socket, &req->client_addr, req->addr_len, 
                      "bio.txt", 1);
    }

    if (req->request_type == 2 || req->request_type == 3) {
        /* Small delay between files */
        usleep(50000);
        send_file_udp(udp_socket, &req->client_addr, req->addr_len, 
                      "bio.pdf", 2);
    }

    /* Send completion packet */
    struct packet done_pkt;
    memset(&done_pkt, 0, sizeof(done_pkt));
    done_pkt.seq_num = htonl(0xFFFFFFFF);  /* Special completion marker */
    done_pkt.total_chunks = htonl(0);
    done_pkt.data_len = htonl(0);
    done_pkt.file_type = htonl(999);
    strcpy(done_pkt.data, "DONE");

    sendto(udp_socket, &done_pkt, sizeof(done_pkt), 0,
           (struct sockaddr *)&req->client_addr, req->addr_len);

    pthread_mutex_lock(&print_mutex);
    printf("[SERVER] UDP client %d transfer complete\n", req->client_id);
    pthread_mutex_unlock(&print_mutex);

    close(udp_socket);
    free(req);
    pthread_exit(NULL);
}

int main() {
    int udp_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int client_counter = 0;

    /* Create UDP socket */
    if ((udp_socket = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    /* Allow socket reuse */
    int opt = 1;
    setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* Configure server address */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    /* Bind socket */
    if (bind(udp_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    printf("  UDP Multi-threaded File Server\n");
    printf("  Port: %d\n", PORT);
    printf("  Chunk Size: %d bytes\n", CHUNK_SIZE);
    printf("[SERVER] Waiting for UDP requests...\n\n");

    /* Main server loop */
    while (1) {
        /* Receive client request */
        int request;
        ssize_t recv_len = recvfrom(udp_socket, &request, sizeof(request), 0,
                                    (struct sockaddr *)&client_addr, &addr_len);

        if (recv_len < 0) {
            perror("Recvfrom failed");
            continue;
        }

        request = ntohl(request);
        client_counter++;

        pthread_mutex_lock(&print_mutex);
        printf("[SERVER] UDP request %d from %s:%d (type: %d)\n", 
               client_counter,
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port),
               request);
        pthread_mutex_unlock(&print_mutex);

        /* Allocate and fill client request structure */
        struct client_request *req = malloc(sizeof(struct client_request));
        if (req == NULL) {
            perror("Malloc failed");
            continue;
        }

        req->client_addr = client_addr;
        req->addr_len = addr_len;
        req->request_type = request;
        req->client_id = client_counter;

        /* Create thread to handle this request */
        pthread_t thread_id;
        if (pthread_create(&thread_id, NULL, handle_client_udp, (void *)req) != 0) {
            perror("Thread creation failed");
            free(req);
            continue;
        }

        pthread_detach(thread_id);
    }

    close(udp_socket);
    pthread_mutex_destroy(&print_mutex);
    return 0;
}
