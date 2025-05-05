#include "controller.hpp"
#include "tcp.hpp"
#include "canvas.hpp"
#include <cmath>
#include <cstdint>
#include <ctime>
#include <optional>


Event::Event(ns_ts timestamp,
             std::function<bool(Controller*)> action)
    : timestamp(timestamp), period(std::nullopt), action(action)
{}

Event::Event(ns_ts timestamp,
             ns_dur period,
             std::function<bool(Controller*)> action)
    : timestamp(timestamp), period(period), action(action)
{}

bool EventQueue::eventCompare(const Event a, const Event b) {
    return a.timestamp < b.timestamp;
}

EventQueue::EventQueue()
    : queue(eventCompare)
{}

void EventQueue::addEvent(Event evnt) {
    this->queue.insert(evnt);
}

std::optional<Event> EventQueue::tryPopEvent(ns_ts cutoff_time) {
    auto next_event_it = this->queue.begin();
    if (next_event_it != this->queue.end() && next_event_it->timestamp < cutoff_time) {
        Event next_event = *next_event_it;
        ns_ts timestamp = next_event.timestamp;
        auto action = next_event.action;
        if (next_event.period.has_value()) {
            ns_dur period = next_event.period.value();
            this->addEvent(Event(timestamp + period, period, action));
        }
        this->queue.erase(next_event_it);
        return next_event;
    } else {
        return std::nullopt;
    }
}

Controller::Controller(VirtualCanvas canvas,
                       std::vector<Client*> clients,
                       LEDTCPServer tcp_server,
                       int64_t ns_per_frame)
    : canvas(canvas),
      clients(clients),
      tcp_server(tcp_server),
      client_conn_info(tcp_server.conn_info),
      event_queue(),
      ns_per_frame(ns_per_frame)
{
    // Add events for all elements
    auto cur_time = std::chrono::system_clock::now();
    for (auto elem : canvas.elementPtrList) {
        int frame_rate = elem->getFrameRate();
        if (frame_rate > 0) {
            ns_dur period = std::chrono::nanoseconds(1'000'000'000 / frame_rate);
            auto nextFrame = [elem](Controller* cont) {
                cv::Mat frame;
                return elem->nextFrame(frame);
            };
            this->event_queue.addEvent(Event(cur_time + period, period, nextFrame));
        }
    }
}

void Controller::frame_wait() {
    struct timespec cur_time;
    clock_gettime(CLOCK_MONOTONIC_RAW, &cur_time);
    int64_t max_sec_as_ns = INT64_MAX / 1'000'000'000;
    int64_t ns_cur_time = (cur_time.tv_sec % max_sec_as_ns) * 1'000'000'000 + cur_time.tv_nsec;
    int64_t ns_wait = this->ns_per_frame - (ns_cur_time % this->ns_per_frame);
    struct timespec wait_remaining;
    struct timespec wait = {ns_wait / 1'000'000'000, ns_wait % 1'000'000'000};
    nanosleep(&wait, &wait_remaining);
}

void Controller::frame_exec(bool debug) {
    this->redraw_all();

    frame_wait();
    auto cur_time = std::chrono::system_clock::now();
    std::optional<Event> event_opt = this->event_queue.tryPopEvent(cur_time);
    while(event_opt.has_value()) {
        Event event = event_opt.value();
        event.action(this);
        event_opt = this->event_queue.tryPopEvent(cur_time);
    }
    this->canvas.pushToCanvas();

    if(debug){ //If this is true, then display the virtual canvas client side. Used for debugging and virtual visualization.

        cv::namedWindow("Virtual Canvas", cv::WINDOW_NORMAL);
        cv::imshow("Virtual Canvas", canvas.getPixelMatrix());
        cv::waitKey(1);
    }
    this->set_leds_all();
}

void Controller::set_leds_all() {
    std::vector<std::pair<const Client*, int>> conns;
    this->client_conn_info->getAllConnected(conns);
    for (auto it : conns) {
        this->tcp_server.set_leds(it.first, it.second, this->canvas);
    }
}

void Controller::redraw_all() {
    std::vector<std::pair<const Client*, int>> conns;
    this->client_conn_info->getAllConnected(conns);
    for (auto it : conns) {
        this->tcp_server.redraw(it.first, it.second);
    }
}
