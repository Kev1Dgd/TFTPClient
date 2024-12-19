#ifndef UTILS_H
#define UTILS_H

#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define BUFFER_SIZE 516  // 2 (opcode) + 2 (block number) + 512 (data)

void create_rrq_buff(char *, char buffer[], size_t *);
void create_wrq_buff(char *, char buffer[], size_t *);
void send_ack(int sockfd, struct sockaddr *, socklen_t, uint16_t); 

#endif