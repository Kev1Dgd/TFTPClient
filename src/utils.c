#include "utils.h"

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

void get_file_content(int sockfd, char buffer[BUFFER_SIZE], FILE *file) {
    struct sockaddr_storage server_addr;
    socklen_t addr_len = sizeof(server_addr);
    int expected_block = 1;
    char print_buffer[512];
    ssize_t bytes_received;

    while (1) {
        bytes_received = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &addr_len);
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
}