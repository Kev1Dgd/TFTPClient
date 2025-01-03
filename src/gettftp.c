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
        sprintf(print_buffer, "Usage: %s HOST [PORT] FILE\n", argv[0]);
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

    get_file_content(sockfd, buffer, file);

    fclose(file);
    close(sockfd);
    freeaddrinfo(res);

    printf("File '%s' received.\n", filename);
    return 0;
}
