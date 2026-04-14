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
#include <optional>
#include <memory>
#include "canvas.hpp"
#include "client.hpp"
#include "protocol.hpp"

class ClientConnInfo {
public:
    std::mutex mut;
    std::map<const Client*, int> connected;
    std::set<const Client*> disconnected;

    explicit ClientConnInfo(std::vector<Client*> clients);

    void setConnected(const Client* c, int socket);
    std::optional<int> getSocket(const Client* c);
    void getAllConnected(std::vector<std::pair<const Client*, int>>& v);
    void getAllDisconnected(std::vector<const Client*>& v);
    void setDisconnected(const Client* c);
    bool isConnected(const Client* c);
};

class LEDTCPServer {
    uint32_t addr;
    uint16_t port;
    int socket;
    ClientConnInfo* conn_info;
    std::thread conn_handling;
    bool is_running = false;

    void handle_conns();

public:
    float brightness_percent;
    LEDTCPServer(uint32_t addr,
                 uint16_t port,
                 int socket,
                 std::vector<Client*> clients,
                 float brightness_percent);
    ~LEDTCPServer();

    LEDTCPServer(const LEDTCPServer&) = delete;
    LEDTCPServer& operator=(const LEDTCPServer&) = delete;

    ClientConnInfo* getConnInfo() const { return conn_info; }

    void start();

    void tcp_send(const Client* c, int socket, void* data, int size);
    MessageHeader tcp_recv_header(int socket);
    void tcp_recv(int socket, void* data, int size);

    void set_leds(const Client* c,
                  int client_socket,
                  VirtualCanvas canvas);
    void redraw(const Client* c, int client_socket);
};

std::shared_ptr<LEDTCPServer> create_server(uint32_t addr,
                                            uint16_t start_port,
                                            uint16_t end_port,
                                            std::vector<Client*> clients,
                                            float brightness_percent);

#endif
