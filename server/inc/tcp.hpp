#pragma once

#include "canvas.hpp"
#include "client.hpp"
#include "protocol.hpp"
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <iterator>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <optional>
#include <set>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h> // for close
#include <unordered_map>
#include <vector>

class ClientConnInfo {
  mutable std::mutex m_mut;
  std::map<uint64_t, int> m_connected;
  std::set<uint64_t> m_disconnected;

  friend class LEDTCPServer;

public:
  explicit ClientConnInfo(const std::vector<std::shared_ptr<Client>> &clients);

  void setConnected(uint64_t addr, int socket);
  std::optional<int> getSocket(uint64_t addr) const;
  void getAllConnected(std::vector<std::pair<uint64_t, int>> &v) const;
  void getAllDisconnected(std::vector<uint64_t> &v) const;
  void setDisconnected(uint64_t addr);
  bool isConnected(uint64_t addr) const;
};

class LEDTCPServer {
  uint32_t m_addr;
  uint16_t m_port;
  int m_socket;
  std::shared_ptr<ClientConnInfo> m_connInfo;
  std::thread m_connHandling;
  bool m_isRunning = false;

  std::unordered_map<uint64_t, std::shared_ptr<const Client>> m_clients;

  void handle_conns();

  float m_brightnessPercent;
  ImageEncoding m_imageEncoding;

public:
  LEDTCPServer(uint32_t addr, uint16_t port, int socket,
               const std::vector<std::shared_ptr<Client>> &clients,
               float brightness_percent, ImageEncoding image_encoding);
  ~LEDTCPServer();

  LEDTCPServer(const LEDTCPServer &) = delete;
  LEDTCPServer &operator=(const LEDTCPServer &) = delete;

  std::shared_ptr<const ClientConnInfo> getConnInfo() const {
    return m_connInfo;
  }

  void start();

  void tcp_send(uint64_t addr, int socket, void *data, int size);
  MessageHeader tcp_recv_header(int socket);
  void tcp_recv(int socket, void *data, int size);

  void set_leds(uint64_t addr, int client_socket, const VirtualCanvas &canvas);
  void redraw(uint64_t addr, int client_socket);
};

std::shared_ptr<LEDTCPServer>
create_server(uint32_t addr, uint16_t port,
              const std::vector<std::shared_ptr<Client>> &clients,
              float brightness_percent, ImageEncoding image_encoding);
