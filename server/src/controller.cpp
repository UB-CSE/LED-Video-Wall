#include "controller.hpp"
#include "canvas.hpp"
#include "tcp.hpp"
#include <cmath>
#include <cstdint>
#include <ctime>
#include <optional>

EventQueue::EventQueue() = default;

void EventQueue::addEvent(Event event) { m_queue.insert(event); }

void EventQueue::clear() { m_queue.clear(); }

std::optional<Event> EventQueue::tryPopEvent(ns_ts cutoff_time) {
  auto eventIt = m_queue.begin();
  if (eventIt == m_queue.end() || eventIt->timestamp >= cutoff_time) {
    return std::nullopt;
  }

  Event event = *eventIt;
  if (event.period.has_value()) {
    ns_dur period = event.period.value();
    addEvent(Event{
        .timestamp = event.timestamp + period,
        .period = period,
        .element = event.element,
    });
  }
  m_queue.erase(m_queue.begin());
  return event;
}

Controller::Controller(VirtualCanvas &canvas,
                       std::shared_ptr<LEDTCPServer> tcpServer,
                       int64_t nsPerFrame)
    : m_canvas(canvas), m_tcpServer(tcpServer),
      m_clientConnInfo((m_tcpServer != nullptr) ? m_tcpServer->getConnInfo()
                                                : nullptr),
      m_nsPerFrame(nsPerFrame) {
  reinitializeCanvasEvents();
}

void Controller::reinitializeCanvasEvents() {
  m_eventQueue.clear();

  auto cur_time = std::chrono::system_clock::now();
  for (std::shared_ptr element : m_canvas.getElements()) {
    int frameRate = element->getFrameRate();
    if (frameRate > 0) {
      ns_dur period = std::chrono::nanoseconds(1'000'000'000 / frameRate);
      m_eventQueue.addEvent(Event{
          .timestamp = cur_time + period,
          .period = period,
          .element = element,
      });
    }
  }
}

void Controller::frameWait() const {
  struct timespec cur_time;
  clock_gettime(CLOCK_MONOTONIC_RAW, &cur_time);
  int64_t max_sec_as_ns = INT64_MAX / 1'000'000'000;
  int64_t ns_cur_time =
      (cur_time.tv_sec % max_sec_as_ns) * 1'000'000'000 + cur_time.tv_nsec;
  int64_t ns_wait = m_nsPerFrame - (ns_cur_time % m_nsPerFrame);
  struct timespec wait_remaining;
  struct timespec wait = {ns_wait / 1'000'000'000, ns_wait % 1'000'000'000};
  nanosleep(&wait, &wait_remaining);
}

void Controller::frameExec(bool showDebugWindow) {
  redrawAll();
  frameWait();

  auto now = std::chrono::system_clock::now();

  std::optional<Event> eventOpt;
  while (eventOpt = m_eventQueue.tryPopEvent(now), eventOpt.has_value()) {
    auto element = eventOpt->element;
    element->acquireNextFrame();
  }

  m_canvas.pushElementsToPixelMatrix();

  if (showDebugWindow) {
    cv::namedWindow("Virtual Canvas", cv::WINDOW_NORMAL);
    cv::imshow("Virtual Canvas", m_canvas.getPixelMatrix());
    cv::waitKey(1);
  }

  setAllLEDs();
}

void Controller::setAllLEDs() {
  if (!m_tcpServer || !m_clientConnInfo) {
    return;
  }

  std::vector<std::pair<uint64_t, int>> conns;
  m_clientConnInfo->getAllConnected(conns);
  for (auto [addr, sock] : conns) {
    m_tcpServer->set_leds(addr, sock, m_canvas);
  }
}

void Controller::redrawAll() {
  if (!m_tcpServer || !m_clientConnInfo) {
    return;
  }

  std::vector<std::pair<uint64_t, int>> conns;
  m_clientConnInfo->getAllConnected(conns);
  for (auto [addr, sock] : conns) {
    m_tcpServer->redraw(addr, sock);
  }
}
