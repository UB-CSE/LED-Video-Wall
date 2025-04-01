#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include "canvas.h"
#include "client.hpp"
#include "tcp.hpp"
#include <bits/types/struct_timespec.h>

class DebugElem {
public:
    VirtualCanvas& canvas;
    Element elem;
    int x;
    int y;
    int dx;
    int dy;
    int max_x;
    int max_y;
    int i;

    DebugElem(VirtualCanvas& canvas);
    void step();
};

class Controller {
public:
    VirtualCanvas& canvas;
    std::vector<Client*> clients;
    LEDTCPServer tcp_server;
    ClientConnInfo* client_conn_info;
    timespec time_per_tick;
    timespec time_per_frame;
    DebugElem debug_elem;

    Controller(VirtualCanvas& canvas,
               std::vector<Client*> clients,
               LEDTCPServer tcp_server,
               timespec time_per_tick,
               timespec time_per_frame);

    void tick_exec();
    void set_leds_all();

private:
    void tick_wait();
};

#endif
