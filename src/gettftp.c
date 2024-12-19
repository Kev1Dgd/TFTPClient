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
    size_t len = 0;

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
    
    create_rrq_buff(filename, buffer, &len);

    if (sendto(sockfd, buffer, len, 0, res->ai_addr, res->ai_addrlen) == -1) {
        close(sockfd);
        freeaddrinfo(res);
        write(STDOUT_FILENO, strerror(errno), strlen(strerror(errno)));
        return 1;
    }

    sprintf(print_buffer, "File: '%s' requested\n", filename);
    write(STDOUT_FILENO, print_buffer, strlen(print_buffer));

    FILE *file = fopen(filename, "wb");
    if (!file) {
        close(sockfd);
        freeaddrinfo(res);
        write(STDOUT_FILENO, strerror(errno), strlen(strerror(errno)));
        return 1;
    }

    struct sockaddr_storage server_addr;
    socklen_t addr_len = sizeof(server_addr);
    int expected_block = 1;

    while (1) {
        bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&server_addr, &addr_len);
        if (bytes_received < 0) {
            break;
        }

        int opcode = (buffer[0] << 8) | buffer[1];
        int block_number = (buffer[2] << 8) | buffer[3];

        if (opcode != 3) {  // Opcode 3= DAT
            sprintf(print_buffer, "Error : opcode=%d received while opcode=3 expected\n", opcode);
            write(STDOUT_FILENO, print_buffer, strlen(print_buffer));
            break;
        }

        if (block_number != expected_block) {
            sprintf(print_buffer, "Unexpected block received (expected: %d, received: %d)\n", expected_block, block_number);
            write(STDOUT_FILENO, print_buffer, strlen(print_buffer));
            break;
        }

        sprintf(print_buffer, "Block %d received (%zd octets).\n", block_number, bytes_received - 4);
        write(STDOUT_FILENO, print_buffer, strlen(print_buffer));

        fwrite(buffer + 4, 1, bytes_received - 4, file);

        send_ack(sockfd, (struct sockaddr *)&server_addr, addr_len, block_number);
        
        sprintf(print_buffer, "ACK sent for block %d\n", block_number);
        write(STDOUT_FILENO, print_buffer, strlen(print_buffer));    

        if (bytes_received < BUFFER_SIZE) {
            break;
        }

        expected_block++;
    }

    fclose(file);
    close(sockfd);
    freeaddrinfo(res);

    printf("File '%s' received.\n", filename);
    return 0;
}
