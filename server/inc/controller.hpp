#pragma once

#include "canvas.hpp"
#include "client.hpp"
#include "tcp.hpp"
#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>
#include <set>

using ns_ts = std::chrono::time_point<std::chrono::system_clock,
                                      std::chrono::nanoseconds>;
using ns_dur = std::chrono::nanoseconds;

struct Event;

struct Event {
  ns_ts timestamp;
  std::optional<ns_dur> period;
  std::shared_ptr<Element> element;
};

class EventQueue {
  static bool eventCompare(const Event &a, const Event &b) {
    return a.timestamp < b.timestamp;
  }

  std::multiset<Event, decltype(eventCompare) *> m_queue{eventCompare};

public:
  EventQueue();

  void clear();
  void addEvent(Event evnt);
  std::optional<Event> tryPopEvent(ns_ts cutoff_time);
};

class Controller {
  VirtualCanvas &m_canvas;
  std::shared_ptr<LEDTCPServer> m_tcpServer;
  std::shared_ptr<const ClientConnInfo> m_clientConnInfo;
  EventQueue m_eventQueue;
  int64_t m_nsPerFrame;

public:
  Controller(VirtualCanvas &canvas, std::shared_ptr<LEDTCPServer> tcpServer,
             int64_t nsPerFrame);

  /**
   * Clears the event queue and re-initializes it with the current canvas
   * elements. This should get called whenever a new element gets added to the
   * canvas or an element's framerate gets changed.
   */
  void reinitializeCanvasEvents();

  /**
   * Periodic function to make and send frames.
   * @param showDebugWindow If true, a GUI window gets shown with the canvas.
   */
  void frameExec(bool showDebugWindow);

  /**
   * Send the current canvas pixel matrix to the microcontroller clients.
   */
  void setAllLEDs();

  /**
   * Send a re-draw message to all microcontroller clients.
   */
  void redrawAll();

private:
  void frameWait() const;
};
