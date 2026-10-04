#include "tcp.hpp"
#include "canvas.hpp"
#include "client.hpp"
#include "protocol.hpp"
#include <chrono>
#include <fcntl.h>
#include <iostream>
#include <iterator>
#include <map>
#include <netdb.h>
#include <netinet/in.h>
#include <opencv2/core.hpp>
#include <optional>
#include <poll.h>
#include <ranges>
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

const int MAX_WAITING_CLIENTS = 256;

void LEDTCPServer::handle_conns() {
  listen(m_socket, MAX_WAITING_CLIENTS);

  while (m_isRunning) {
    int client_socket = accept(m_socket, nullptr, nullptr);
    if (client_socket < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        continue;
      }

      spdlog::error("[LEDTCPServer] accept failed: {}", strerror(errno));
      break;
    }

    int flags = fcntl(client_socket, F_GETFL, 0);
    if (flags == -1) {
      close(client_socket);
      continue;
    }
    if (fcntl(client_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
      close(client_socket);
      continue;
    }
    CheckInMessage msg{};
    MessageHeader *header = &msg.header;
    *header = tcp_recv_header(client_socket);
    if (msg.header.op_code != OperationCode::CHECK_IN ||
        msg.header.size != sizeof(CheckInMessage)) {
      spdlog::error("[LEDTCPServer] expected check-in message, got invalid "
                    "op-code or message size");
      close(client_socket);
      continue;
    }
    tcp_recv(client_socket, &(msg.mac_address),
             sizeof(msg) - sizeof(MessageHeader));
    uint64_t mac_addr = 0;
    memcpy(&mac_addr, &(msg.mac_address), 6);

    const std::string macAddrString =
        std::format("{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}",
                    unsigned(msg.mac_address[0]), unsigned(msg.mac_address[1]),
                    unsigned(msg.mac_address[2]), unsigned(msg.mac_address[3]),
                    unsigned(msg.mac_address[4]), unsigned(msg.mac_address[5]));
    spdlog::info("[LEDTCPServer] got message from {}", macAddrString);

    auto it = m_clients.find(mac_addr);
    if (!(it == m_clients.end())) {
      const std::shared_ptr<const Client> &c = it->second;

      // If the client reconnects before its old socket has disconnected,
      // close the old socket and mark the client as disconnected.
      auto socket_opt = m_connInfo->getSocket(c->macAddr);
      if (socket_opt.has_value()) {
        int s = socket_opt.value();
        m_connInfo->setDisconnected(c->macAddr);
        close(s);
      }

      spdlog::info("[LEDTCPServer] accepted client {} (socket {})",
                   macAddrString, client_socket);

      uint8_t num_pins = c->matConnections.size();
      std::vector<PinInfo> pin_info;
      for (const MatricesConnection &conn : c->matConnections) {
        uint32_t max_leds = 0;
        for (const std::shared_ptr<LEDMatrix> &mat : conn.matrices) {
          max_leds += mat->spec->width * mat->spec->height;
        }
        LEDType led_type = (conn.pin < 0) ? LEDType::HUB75 : LEDType::WS2811;
        pin_info.push_back((PinInfo){conn.pin, led_type, max_leds});
      }
      std::vector<uint8_t> encoded_msg =
          encode_set_config(pin_info, m_imageEncoding);
      struct pollfd pfd = {client_socket, POLLOUT, -1};
      poll(&pfd, 1, -1);
      send(client_socket, encoded_msg.data(), encoded_msg.size(), 0);
      spdlog::info("[LEDTCPServer] sent set_config to {}", macAddrString);
      m_connInfo->setConnected(c->macAddr, client_socket);
    } else {
      spdlog::error("[LEDTCPServer] did not recognize MAC address {}",
                    macAddrString);
      close(client_socket);
    }
  }
}

std::shared_ptr<LEDTCPServer>
create_server(uint32_t addr, uint16_t port,
              const std::vector<std::shared_ptr<Client>> &clients,
              float brightness_percent, ImageEncoding image_encoding) {
  struct protoent *protocol_entry = getprotobyname("tcp");
  const int tcp_protocol_num = protocol_entry->p_proto;

  int server_socket = socket(AF_INET, SOCK_STREAM, tcp_protocol_num);
  if (server_socket == -1) {
    spdlog::error("[LEDTCPServer] socket() failed: ", strerror(errno));
    return nullptr;
  }

  int enable = 1;
  setsockopt(server_socket, SOL_SOCKET, SO_REUSEPORT, &enable, sizeof(enable));

  struct sockaddr_in s_addr;
  s_addr.sin_family = AF_INET;
  s_addr.sin_port = htons(port);
  s_addr.sin_addr.s_addr = addr;

  int res = bind(server_socket, (struct sockaddr *)&s_addr, sizeof(s_addr));
  if (res == -1) {
    spdlog::error("[LEDTCPServer] bind() failed: ", strerror(errno));
    return nullptr;
  }

  // Set the socket to non-blocking

  int flags = fcntl(server_socket, F_GETFL, 0);
  if (flags < 0) {
    spdlog::error("[LEDTCPServer] failed to get socket flags: ",
                  strerror(errno));
    close(server_socket);
    return nullptr;
  }

  if (fcntl(server_socket, F_SETFL, flags | O_NONBLOCK) < 0) {
    spdlog::error("[LEDTCPServer] failed to set socket to non-blocking: ",
                  strerror(errno));
    close(server_socket);
    return nullptr;
  }

  return std::make_shared<LEDTCPServer>(addr, port, server_socket, clients,
                                        brightness_percent, image_encoding);
}

LEDTCPServer::LEDTCPServer(uint32_t addr, uint16_t port, int sock,
                           const std::vector<std::shared_ptr<Client>> &clients,
                           float brightnessPercent, ImageEncoding imageEncoding)
    : m_addr(addr), m_port(port), m_socket(sock),
      m_connInfo(std::make_shared<ClientConnInfo>(clients)),
      m_brightnessPercent(brightnessPercent), m_imageEncoding(imageEncoding) {

  m_brightnessPercent = std::clamp(m_brightnessPercent, 1.f, 100.f);

  for (const std::shared_ptr<Client> &client : clients) {
    m_clients[client->macAddr] = client;
  }
}

LEDTCPServer::~LEDTCPServer() {
  if (m_isRunning) {
    m_isRunning = false;
    m_connHandling.join();
  }
  for (const int clientSocket : m_connInfo->m_connected | std::views::values) {
    close(clientSocket);
  }
  close(m_socket);

  spdlog::error("[LEDTCPServer] stopped (port {})", m_port);
}

void LEDTCPServer::start() {
  if (!m_isRunning) {
    m_isRunning = true;
    m_connHandling = std::thread(&LEDTCPServer::handle_conns, this);
  }
}

ClientConnInfo::ClientConnInfo(
    const std::vector<std::shared_ptr<Client>> &clients) {
  for (const std::shared_ptr<Client> &c : clients) {
    m_disconnected.insert(c->macAddr);
  }
}

void ClientConnInfo::setConnected(uint64_t addr, int sock) {
  std::lock_guard lock(m_mut);
  if (!m_connected.contains(addr)) {
    m_disconnected.erase(addr);
    m_connected[addr] = sock;
  }
}

std::optional<int> ClientConnInfo::getSocket(uint64_t addr) const {
  std::lock_guard lock(m_mut);
  auto conn = m_connected.find(addr);
  if (conn != m_connected.end()) {
    return conn->second;
  }
  return std::nullopt;
}

void ClientConnInfo::getAllConnected(
    std::vector<std::pair<uint64_t, int>> &v) const {
  std::lock_guard lock(m_mut);
  for (const auto &[addr, sock] : m_connected) {
    v.emplace_back(addr, sock);
  }
}

void ClientConnInfo::getAllDisconnected(std::vector<uint64_t> &v) const {
  std::lock_guard lock(m_mut);
  for (uint64_t addr : m_disconnected) {
    v.push_back(addr);
  }
}

void ClientConnInfo::setDisconnected(uint64_t addr) {
  std::lock_guard lock(m_mut);
  if (!m_disconnected.contains(addr)) {
    m_connected.erase(addr);
    m_disconnected.insert(addr);
  }
}

bool ClientConnInfo::isConnected(uint64_t addr) const {
  std::lock_guard lock(m_mut);
  return m_connected.contains(addr);
}

void LEDTCPServer::tcp_send(uint64_t addr, int s, void *data, int size) {
  ssize_t total_sent = 0;
  while (total_sent < size) {
    const ssize_t sent =
        send(s, (char *)data + total_sent, size - total_sent, MSG_NOSIGNAL);
    if (sent < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        continue;
      }
      spdlog::error("[LEDTCPServer] error sending: {}", strerror(errno));
      if (errno == ECONNRESET || errno == EPIPE) {
        auto socket_opt = m_connInfo->getSocket(addr);
        if (socket_opt.has_value()) {
          close(socket_opt.value());
        }
        m_connInfo->setDisconnected(addr);
      }
      break;
    }
    total_sent += sent;
  }
}

MessageHeader LEDTCPServer::tcp_recv_header(int sock) {
  MessageHeader header{};
  struct pollfd pfd = {.fd = sock, .events = POLLIN, .revents = 0};
  ssize_t total = 0;
  while (total < static_cast<ssize_t>(sizeof(MessageHeader))) {
    poll(&pfd, 1, -1);
    const ssize_t recvSize =
        recv(sock, (char *)&header + total, sizeof(header) - total, 0);
    if (recvSize < 0) {
      spdlog::error("[LEDTCPServer] error receiving header: {}",
                    strerror(errno));
    } else {
      total += recvSize;
    }
  }
  return header;
}

void LEDTCPServer::tcp_recv(int sock, void *data, int size) {
  struct pollfd pfd = {.fd = sock, .events = POLLIN, .revents = 0};
  poll(&pfd, 1, -1);
  ssize_t total = 0;
  while (total < size) {
    poll(&pfd, 1, -1);
    const ssize_t recvSize = recv(sock, (char *)data + total, size - total, 0);
    if (recvSize < 0) {
      spdlog::error("[LEDTCPServer] error receiving: {}", strerror(errno));
    } else {
      total += recvSize;
    }
  }
}

void LEDTCPServer::set_leds(uint64_t addr, int client_socket,
                            const VirtualCanvas &canvas) {
  std::vector<LEDsBatchEntryData> leds_batches;
  for (const MatricesConnection &conn : m_clients[addr]->matConnections) {
    uint64_t total_size = 0;
    int8_t pin = conn.pin;

    std::vector<std::shared_ptr<LEDsPixelData>> matrices;

    // This loop processes each submatrix (corresponding to a ledstrip) one at a
    // time. pixel_buf points to the next part of the temp_buf for the current
    // submatrix. Note that pixel_buf is also updated at the end of each
    // iteration of this loop.
    for (const std::shared_ptr<LEDMatrix> &ledmat : conn.matrices) {
      const size_t pixel_array_size = ledmat->getRGB24PixelArraySize();
      total_size += pixel_array_size;

      std::shared_ptr<LEDsPixelData> outMatrix(
          reinterpret_cast<LEDsPixelData *>(
              new uint8_t[2 * sizeof(uint16_t) + pixel_array_size]));

      uint32_t width = ledmat->spec->width;
      uint32_t height = ledmat->spec->height;
      // swap width and height if rotated +/-90 degrees
      Rotation rot = ledmat->pos.rot;
      if (rot == Rotation::LEFT || rot == Rotation::RIGHT) {
        std::swap(width, height);
      }
      uint32_t x = ledmat->pos.x;
      uint32_t y = ledmat->pos.y;

      outMatrix->width = width;
      outMatrix->height = height;

      uint8_t *pixel_buf = outMatrix->pixel_data;

      cv::Mat sub_cvmat =
          canvas.getPixelMatrix()(cv::Rect(x, y, width, height)).clone();
      sub_cvmat.convertTo(sub_cvmat, -1, m_brightnessPercent / 100.0);

      if (rot == Rotation::LEFT) {
        cv::rotate(sub_cvmat, sub_cvmat, cv::ROTATE_90_CLOCKWISE);
      } else if (rot == Rotation::RIGHT) {
        cv::rotate(sub_cvmat, sub_cvmat, cv::ROTATE_90_COUNTERCLOCKWISE);
      } else if (rot == Rotation::DOWN) {
        cv::rotate(sub_cvmat, sub_cvmat, cv::ROTATE_180);
      }

      const uint8_t *data = sub_cvmat.data;
      // todo: brightness_reduction should be configurable!
      // Added brightness percent and removed brightness_reduction;
      const uint32_t num_leds = pixel_array_size / 3;
      for (uint32_t i = 0; (i < num_leds); ++i) {
        uint32_t a = i * 3;
        if (pin < 0 || (i / width) % 2 != 0) {
          pixel_buf[a + 2] = data[a];     // b
          pixel_buf[a + 1] = data[a + 1]; // g
          pixel_buf[a] = data[a + 2];     // r
        } else {
          uint32_t irem = i % width;
          uint32_t b = (((width - 1) - irem) + (i - irem)) * 3;
          pixel_buf[a + 2] = data[b];     // b
          pixel_buf[a + 1] = data[b + 1]; // g
          pixel_buf[a] = data[b + 2];     // r
        }
      }

      matrices.push_back(outMatrix);
    }
    uint32_t num_leds_total = total_size / 3;
    LEDsBatchEntryData batch{
        .gpio_pin = pin,
        .num_leds = num_leds_total,
        .matrices = matrices,
    };
    leds_batches.push_back(batch);
  }
  std::vector<uint8_t> msg_buf =
      encode_set_leds_batched(leds_batches, m_imageEncoding);
  this->tcp_send(addr, client_socket, msg_buf.data(),
                 static_cast<int>(msg_buf.size()));
}

void LEDTCPServer::redraw(uint64_t addr, int client_socket) {
  std::vector<uint8_t> msg_buf = encode_redraw();
  this->tcp_send(addr, client_socket, msg_buf.data(),
                 static_cast<int>(msg_buf.size()));
}
