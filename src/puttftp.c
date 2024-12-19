#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

#include "utils.h"

#define BUFFER_SIZE 516  // 2 (opcode) + 2 (block number) + 512 (data)

int main(int argc, char *argv[]) {
    char print_buffer[512];
    char *port, *hostname, *filename;

    if (argc == 4) {
        hostname = argv[1];
        port = argv[2];
        filename = argv[3];
    } else if (argc != 3) {
        sprintf(print_buffer, "Usage: %s HOST FILE\n", argv[0]);
        write(STDOUT_FILENO, print_buffer, strlen(print_buffer));
        return 1;
    } else {
        hostname = argv[1];
        port = "69";
        filename = argv[2];
    }

    struct addrinfo hints, *res;
    int sockfd;
    ssize_t bytes_received;
    char buffer[BUFFER_SIZE];
    size_t len;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    if (getaddrinfo(hostname, port, &hints, &res) != 0) {
        write(STDOUT_FILENO, strerror(errno), strlen(strerror(errno)));
        return 1;
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        freeaddrinfo(res);
        write(STDOUT_FILENO, strerror(errno), strlen(strerror(errno)));
        return 1;
    }

    create_wrq_buff(filename, buffer, &len);
    if (sendto(sockfd, buffer, len, 0, res->ai_addr, res->ai_addrlen) == -1) {
        close(sockfd);
        freeaddrinfo(res);
        write(STDOUT_FILENO, strerror(errno), strlen(strerror(errno)));
        return 1;
    }

    sprintf(print_buffer, "File: '%s' upload started\n", filename);
    write(STDOUT_FILENO, print_buffer, strlen(print_buffer));

    FILE *file = fopen(filename, "rb");
    if (!file) {
        close(sockfd);
        freeaddrinfo(res);
        write(STDOUT_FILENO, strerror(errno), strlen(strerror(errno)));
        return 1;
    }

    struct sockaddr_storage server_addr;
    socklen_t addr_len = sizeof(server_addr);
    int block_number = 1;

    while (1) {
        size_t data_size = fread(buffer + 4, 1, 512, file);
        if (data_size == 0 && feof(file)) {
            break;
        }

        buffer[0] = 0x00;
        buffer[1] = 0x03; // Opcode DATA = 3
        buffer[2] = (block_number >> 8) & 0xFF; 
        buffer[3] = block_number & 0xFF;

        if (sendto(sockfd, buffer, data_size + 4, 0, res->ai_addr, res->ai_addrlen) == -1) {
            perror("Error sending DATA packet");
            break;
        }

        bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&server_addr, &addr_len);
        if (bytes_received < 4 || buffer[1] != 0x04) {
            write(STDOUT_FILENO, "Error receiving ACK\n", strlen("Error receiving ACK\n"));
            break;
        }

        block_number++;

        if (data_size < 512) { 
            break;
        }
    }

    fclose(file);
    close(sockfd);
    freeaddrinfo(res);

    sprintf(print_buffer, "File '%s' uploaded successfully.\n", filename);
    write(STDOUT_FILENO, print_buffer, strlen(print_buffer));
    return 0;
}
