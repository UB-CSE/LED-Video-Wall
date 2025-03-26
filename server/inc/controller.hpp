#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include "canvas.h"
#include "client.hpp"
#include "tcp.hpp"

class Controller {
public:
    VirtualCanvas& canvas;
    std::vector<Client*> clients;
    LEDTCPServer tcp_server;
    ClientConnInfo* client_conn_info;

    Controller(VirtualCanvas& canvas,
               std::vector<Client*> clients,
               LEDTCPServer tcp_server);

    void set_leds_all();
};

#endif
