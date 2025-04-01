#include "controller.hpp"
#include "tcp.hpp"
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
    if (i % 10 == 0) {
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
}

Controller::Controller(VirtualCanvas& canvas,
                       std::vector<Client*> clients,
                       LEDTCPServer tcp_server,
                       timespec time_per_tick,
                       timespec time_per_frame)
    : canvas(canvas),
      clients(clients),
      tcp_server(tcp_server),
      client_conn_info(tcp_server.conn_info),
      time_per_tick(time_per_tick),
      time_per_frame(time_per_frame),
      debug_elem(DebugElem(canvas))
{
    std::vector<Element> elemVec = {debug_elem.elem};
    canvas.addElementToCanvas(elemVec);
}

void Controller::tick_wait() {
    struct timespec wait_remaining;
    struct timespec wait_for;
    clock_gettime(CLOCK_MONOTONIC_RAW, &wait_for);
    wait_for.tv_sec = wait_for.tv_sec - this->time_per_tick.tv_sec;
    wait_for.tv_nsec = wait_for.tv_nsec - this->time_per_tick.tv_nsec;
    do {
        nanosleep(&wait_for, &wait_remaining);
    } while (wait_remaining.tv_sec != 0 && wait_remaining.tv_nsec != 0);
}

void Controller::tick_exec() {
    // Todo: send redraw command to all clients.
    this->canvas.updateCanvas();
    this->debug_elem.step();
    tick_wait();
    this->set_leds_all();
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
