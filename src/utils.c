#include "utils.h"
#include <stdio.h>

void create_rrq_buff(char *filename, char buffer[BUFFER_SIZE], size_t *len) {
    buffer[(*len)++] = 0x00;
    buffer[(*len)++] = 0x01;  // Opcode 1 = RRQ

    memcpy(&buffer[*len], filename, strlen(filename));
    *len += strlen(filename);
    buffer[(*len)++] = 0x00;

    const char *mode = "octet"; // NETASCII doesnt work for images
    memcpy(&buffer[*len], mode, strlen(mode));
    *len += strlen(mode);
    buffer[(*len)++] = 0x00;
}

void create_wrq_buff(char *filename, char buffer[BUFFER_SIZE], size_t *len) {
    *len = 0;
    buffer[(*len)++] = 0x00;
    buffer[(*len)++] = 0x02;  // Opcode 2 = WRQ

    memcpy(&buffer[*len], filename, strlen(filename));
    *len += strlen(filename);
    buffer[(*len)++] = 0x00;

    const char *mode = "octet";
    memcpy(&buffer[*len], mode, strlen(mode));
    *len += strlen(mode);
    buffer[(*len)++] = 0x00;
}


void send_ack(int sockfd, struct sockaddr *server_addr, socklen_t addr_len, uint16_t block_number) {
    char ack[4];
    ack[0] = 0x00;
    ack[1] = 0x04;  // Opcode 4 = ACK
    ack[2] = (block_number >> 8) & 0xFF;
    ack[3] = block_number & 0xFF;

    sendto(sockfd, ack, sizeof(ack), 0, server_addr, addr_len);
}