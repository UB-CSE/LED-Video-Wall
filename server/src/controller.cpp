#include "controller.hpp"
#include "tcp.hpp"
#include <bits/types/struct_timespec.h>
#include <cmath>
#include <cstdint>
#include <ctime>

DebugElem::DebugElem(VirtualCanvas& canvas)
    : canvas(canvas),
      elem(Element("images/img5x5_1.jpg", 1000, cv::Point(0, 0))),
      x(0), y(0), dx(1), dy(1),
      max_x(canvas.getDimensions().width),
      max_y(canvas.getDimensions().height),
      i(0)
{}

void DebugElem::step() {
    i++;
    canvas.removeElementFromCanvas(this->elem.getId());
    if (x >= max_x - 5) {
        dx = -1;
    } else if (x <= 0) {
        dx = 1;
    }
    if (y >= max_y - 5) {
        dy = -1;
    } else if (y <= 0) {
        dy = 1;
    }
    x += dx;
    y += dy;
    elem.setLocation(cv::Point(x, y));
    std::vector<Element> elemVec = {this->elem};
    this->canvas.addElementToCanvas(elemVec);
}

Controller::Controller(VirtualCanvas& canvas,
                       std::vector<Client*> clients,
                       LEDTCPServer tcp_server,
                       int64_t ns_per_tick,
                       int64_t ns_per_frame)
    : canvas(canvas),
      clients(clients),
      tcp_server(tcp_server),
      client_conn_info(tcp_server.conn_info),
      ns_per_tick(ns_per_tick),
      ns_per_frame(ns_per_frame),
      tick(0),
      debug_elem(DebugElem(canvas))
{
    this->ticks_per_frame = ns_per_frame / ns_per_tick;
    std::vector<Element> elemVec = {debug_elem.elem};
    canvas.addElementToCanvas(elemVec);
}

void Controller::tick_wait() {
    struct timespec cur_time;
    clock_gettime(CLOCK_MONOTONIC_RAW, &cur_time);
    int64_t max_sec_as_ns = INT64_MAX / 1'000'000'000;
    int64_t ns_cur_time = (cur_time.tv_sec % max_sec_as_ns) * 1'000'000'000 + cur_time.tv_nsec;
    int64_t ns_wait = ns_cur_time % this->ns_per_tick;
    struct timespec wait_remaining;
    struct timespec wait = {ns_wait / 1'000'000'000, ns_wait % 1'000'000'000};
    nanosleep(&wait, &wait_remaining);
}

bool Controller::is_frame_tick() {
    return (this->tick % this->ticks_per_frame) == 0;
}

void Controller::tick_exec() {
    // Todo: send redraw command to all clients.
    this->canvas.updateCanvas();
    if (this->is_frame_tick()) {
        this->debug_elem.step();
    }
    tick_wait();
    if (this->is_frame_tick()) {
        std::cout << "set_leds_all\n";
        this->set_leds_all();
    }
    this->tick++;
}

void Controller::set_leds_all() {
    std::vector<std::pair<const Client*, int>> conns;
    this->client_conn_info->getAllConnected(conns);
    for (auto it : conns) {
        for (MatricesConnection conn : it.first->mat_connections) {
            uint8_t pin = conn.pin;
            for (LEDMatrix* mat : conn.matrices) {
                this->tcp_server.set_leds(it.first,
                                          it.second,
                                          this->canvas.getPixelMatrix(),
                                          mat,
                                          pin,
                                          8);
            }
        }
    }
}
