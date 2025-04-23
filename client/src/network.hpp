#ifndef NETWORK_H
#define NETWORK_H

#define SERVER_IP "yoshi.cse.buffalo.edu"
#define SERVER_PORT_START 7070
#define SERVER_PORT_END 7074

#define CHECK_IN_DELAY_MS 500
#define RECV_TIMEOUT_SEC 5

#include <unistd.h>

int checkin(int *out_sockfd);
int parse_tcp_message(int sockfd, uint8_t **buffer, uint32_t *buffer_size);

#endif
