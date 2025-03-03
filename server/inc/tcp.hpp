#ifndef TCP_H
#define TCP_H

#include "client.hpp"

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <iterator>
#include <string.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h> // for close
#include <vector>

class LEDTCPServer {
public:
    uint32_t addr;
    uint16_t port;
    int socket;

    LEDTCPServer(uint32_t addr, uint16_t port, int socket);
    void wait_all_join(std::vector<Client*> clients);
};

std::optional<LEDTCPServer> create_server(uint32_t addr, uint16_t port);

void tcp_set_leds(int client_socket, const cv::Mat &cvmat, LEDMatrix* ledmat, uint8_t pin, uint8_t bit_depth);

#endif
