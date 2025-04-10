#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include "canvas.h"
#include "client.hpp"
#include "tcp.hpp"
#include <bits/types/struct_timespec.h>
#include <cstdint>

// class DebugElem {
// public:
//     VirtualCanvas& canvas;
//     Element elem;
//     int x;
//     int y;
//     int dx;
//     int dy;
//     int max_x;
//     int max_y;
//     int i;

//     DebugElem(VirtualCanvas& canvas);
//     void step();
// };

class Controller {
public:
    MPI_Win win;
    cv::Size canvas_size;
    uchar* pixel_array;
    std::vector<Client*> clients;
    LEDTCPServer tcp_server;
    ClientConnInfo* client_conn_info;
    int64_t ns_per_frame;
    // DebugElem debug_elem;

    Controller(MPI_Win win,
               cv::Size canvas_size,
               std::vector<Client*> clients,
               LEDTCPServer tcp_server,
               int64_t ns_per_frame);

    void frame_exec();
    void set_leds_all();

private:
    void frame_wait();
};

#endif
