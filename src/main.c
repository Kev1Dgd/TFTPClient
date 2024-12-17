#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

#define TFTP_PORT "1069"
#define BUFFER_SIZE 516  // 2 (opcode) + 2 (block number) + 512 (data)

void send_ack(int sockfd, struct sockaddr *server_addr, socklen_t addr_len, uint16_t block_number) {
    char ack[4];
    ack[0] = 0x00;
    ack[1] = 0x04;  // Opcode 4 pour ACK
    ack[2] = (block_number >> 8) & 0xFF;
    ack[3] = block_number & 0xFF;

    sendto(sockfd, ack, sizeof(ack), 0, server_addr, addr_len);
    printf("ACK envoyé pour le bloc %d\n", block_number);
}

void print(const char *str) {
    size_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    write(STDOUT_FILENO, str, len);
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s HOST FILE\n", argv[0]);
        return 1;
    }

    const char *hostname = argv[1];
    const char *filename = argv[2];

    struct addrinfo hints, *res;
    int sockfd;
    ssize_t bytes_sent, bytes_received;
    char buffer[BUFFER_SIZE];
    size_t len = 0;

    // Configuration de getaddrinfo
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    if (getaddrinfo(hostname, TFTP_PORT, &hints, &res) != 0) {
        perror("getaddrinfo");
        return 1;
    }

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0) {
        perror("socket");
        freeaddrinfo(res);
        return 1;
    }

    buffer[len++] = 0x00;
    buffer[len++] = 0x01;  // Opcode pour RRQ

    memcpy(&buffer[len], filename, strlen(filename));
    len += strlen(filename);
    buffer[len++] = 0x00;

    const char *mode = "octet";
    memcpy(&buffer[len], mode, strlen(mode));
    len += strlen(mode);
    buffer[len++] = 0x00;

    bytes_sent = sendto(sockfd, buffer, len, 0, res->ai_addr, res->ai_addrlen);
    if (bytes_sent < 0) {
        perror("sendto");
        close(sockfd);
        freeaddrinfo(res);
        return 1;
    }
    printf("Requête RRQ envoyée pour le fichier '%s'.\n", filename);

    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("fopen");
        close(sockfd);
        freeaddrinfo(res);
        return 1;
    }

    struct sockaddr_storage server_addr;
    socklen_t addr_len = sizeof(server_addr);
    uint16_t expected_block = 1;

    while (1) {
        bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&server_addr, &addr_len);
        if (bytes_received < 0) {
            perror("recvfrom");
            break;
        }

        uint16_t opcode = (buffer[0] << 8) | buffer[1];
        uint16_t block_number = (buffer[2] << 8) | buffer[3];

        if (opcode != 3) {  // Opcode pour DAT
            fprintf(stderr, "Paquet inconnu reçu (opcode %d)\n", opcode);
            break;
        }

        if (block_number != expected_block) {
            fprintf(stderr, "Bloc inattendu reçu (attendu: %d, reçu: %d)\n", expected_block, block_number);
            break;
        }

        printf("Réception du bloc %d (%zd octets).\n", block_number, bytes_received - 4);

        fwrite(buffer + 4, 1, bytes_received - 4, file);

        send_ack(sockfd, (struct sockaddr *)&server_addr, addr_len, block_number);

        if (bytes_received < BUFFER_SIZE) {
            printf("Dernier bloc reçu. Fin du transfert.\n");
            break;
        }

        expected_block++;
    }

    fclose(file);
    close(sockfd);
    freeaddrinfo(res);

    printf("Fichier '%s' reçu avec succès.\n", filename);
    return 0;
}
