#ifndef TCP_H
#define TCP_H

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
#include <set>
#include <thread>
#include "client.hpp"

class ClientConnInfo {
public:
    std::mutex mut;
    std::map<const Client*, int> connected;
    std::set<const Client*> disconnected;

    ClientConnInfo(std::vector<Client*> clients);

    void setConnected(const Client* c, int socket);
    void setDisconnected(const Client* c);
    bool isConnected(const Client* c);
};

class LEDTCPServer {
public:
    uint32_t addr;
    uint16_t port;
    int socket;
    ClientConnInfo* conn_info;
    std::thread& conn_handling;

    LEDTCPServer(uint32_t addr,
                 uint16_t port,
                 int socket,
                 std::vector<Client*> clients,
                 void(*handle_conns)(int socket, ClientConnInfo* conn_info));
};

std::optional<LEDTCPServer> create_server(uint32_t addr,
                                          uint16_t start_port,
                                          uint16_t end_port,
                                          std::vector<Client*> clients);

void tcp_set_leds(int client_socket, const cv::Mat &cvmat, LEDMatrix* ledmat, uint8_t pin, uint8_t bit_depth);

#endif
