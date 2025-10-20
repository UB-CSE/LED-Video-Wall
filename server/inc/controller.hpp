#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include "canvas.hpp"
#include "client.hpp"
#include "tcp.hpp"
#include <chrono>
#include <cstdint>
#include <optional>

using ns_ts = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>;
using ns_dur = std::chrono::nanoseconds;

class Controller;

class Event {
public:
    ns_ts timestamp;
    std::optional<ns_dur> period;
    std::function<bool(Controller*)> action;

    Event(ns_ts timestamp,
          std::function<bool(Controller*)> action);

    Event(ns_ts timestamp,
          ns_dur period,
          std::function<bool(Controller*)> action);
};

class EventQueue {
private:
    static bool eventCompare(const Event a, const Event b);
public:
    EventQueue();

    std::multiset<Event, decltype(eventCompare)*> queue;

    void addEvent(Event evnt);
    std::optional<Event> tryPopEvent(ns_ts cutoff_time);
};

class Controller {
public:
    VirtualCanvas &canvas;
    std::vector<Client*> clients;
    LEDTCPServer tcp_server;
    ClientConnInfo* client_conn_info;
    EventQueue event_queue;
    int64_t ns_per_frame;

    Controller(VirtualCanvas &canvas,
               std::vector<Client*> clients,
               LEDTCPServer tcp_server,
               int64_t ns_per_frame);

    void frame_exec(bool debug);
    void set_leds_all();
    void redraw_all();

private:
    void frame_wait();
};

#endif
