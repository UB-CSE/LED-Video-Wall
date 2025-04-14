#include "tcp.hpp"
#include "canvas.h"
#include "client.hpp"
#include <asm-generic/errno.h>
#include <asm-generic/socket.h>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <iterator>
#include <optional>
#include <string.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <map>
#include <thread>
#include <utility>
#include <poll.h>
#include "opencv2/core.hpp"
#include "protocol.hpp"

const int MAX_WAITING_CLIENTS = 256;

void handle_conns(int socket, LEDTCPServer* server) {

    listen(socket, MAX_WAITING_CLIENTS);

    std::map<uint64_t, const Client*> mac_to_client;

    std::vector<const Client*> clients;
    server->conn_info->getAllDisconnected(clients);
    for (auto c : clients) {
        mac_to_client[c->mac_addr] = c;
    }

    while (1) {
        int client_socket = accept4(socket, NULL, NULL, SOCK_NONBLOCK);
        CheckInMessage msg;
        MessageHeader* header = &msg.header;
        *header = server->tcp_recv_header(client_socket);
        if (msg.header.op_code != OP_CHECK_IN || msg.header.size != sizeof(CheckInMessage)) {
            std::cerr << "Expected check-in message, got invalid op-code or message size.\n";
            close(client_socket);
            break;
        }
        server->tcp_recv(client_socket,
                         &(msg.mac_address),
                         sizeof(msg) - sizeof(MessageHeader));
        uint64_t mac_addr = 0;
        std::cout << &(msg.mac_address) << "," << &msg + sizeof(MessageHeader) << "," << &msg << "," << sizeof(MessageHeader) << "\n";
        memcpy(&mac_addr, &(msg.mac_address), 6);

        // if c is the value representing the end of the iterator, it is not present
        std::cout << "Got message from " << mac_addr << "\n";
        auto it = mac_to_client.find(mac_addr);
        if (!(it == mac_to_client.end())) {
            const Client* c = it->second;

            // If the client reconnects before its old socket has disconnected,
            // close the old socket and mark the client as disconnected.
            auto socket_opt = server->conn_info->getSocket(c);
            if (socket_opt.has_value()) {
                int socket = socket_opt.value();
                server->conn_info->setDisconnected(c);
                close(socket);
            }

            std::cout << "Accepted client\n";
            std::cout << "socket: " << client_socket << "\n";

            uint8_t num_pins = c->mat_connections.size();
            std::vector<PinInfo> pin_info;
            for (MatricesConnection conn : c->mat_connections) {
                uint32_t max_leds = 0;
                for (LEDMatrix* mat : conn.matrices) {
                    max_leds += mat->spec->width * mat->spec->height;
                }
                pin_info.push_back((PinInfo){
                        conn.pin,
                        COLOR_ORDER_GRB,
                        max_leds,
                        LED_TYPE_WS2811
                    });
            }
            const PinInfo* inf = pin_info.data();
            uint32_t out_size;
            uint8_t* msg = encode_set_config(3, 10, num_pins, inf, &out_size);
            struct pollfd pfd = {client_socket, POLLOUT, -1};
            poll(&pfd, 1, -1);
            send(client_socket, msg, out_size, 0);
            std::cout << "Sent set_config to " << mac_addr << "\n";
            server->conn_info->setConnected(c, client_socket);
        } else {
            std::cerr << "Did not recognize MAC address!\n";
            close(client_socket);
        }
    }
}

std::optional<LEDTCPServer> create_server(uint32_t addr,
                                          uint16_t start_port,
                                          uint16_t end_port,
                                          std::vector<Client*> clients) {
    struct protoent* protocol_entry = getprotobyname("tcp");
    const int tcp_protocol_num = protocol_entry->p_proto;
    
    int server_socket = socket(AF_INET, SOCK_STREAM, tcp_protocol_num);
    if (server_socket == -1) {
        std::cerr << "Bad socket!\n";
        return std::nullopt;
    }

    int enable = 1;
    setsockopt(server_socket, tcp_protocol_num, SO_REUSEPORT, &enable, sizeof(enable));

    uint16_t port;
    for (port = start_port; port <= end_port; port ++) {
        struct sockaddr_in s_addr;
        s_addr.sin_family = AF_INET;
        s_addr.sin_port = htons(port);
        s_addr.sin_addr.s_addr = addr;

        int res = bind(server_socket, (struct sockaddr *)&s_addr, sizeof(s_addr));
        if (res == -1) {
            std::cerr << "Failed bind: " << strerror(errno) << "\n";
            if (port == end_port) {
                std::cerr << "ERROR: could not bind to any of the ports in the range "
                          << start_port << " to "
                          << end_port << "\n";
                return std::nullopt;
            }
        } else {
            break;
        }
    }

    return LEDTCPServer(addr, port, server_socket, clients, handle_conns);
}

LEDTCPServer::LEDTCPServer(uint32_t addr,
                           uint16_t port,
                           int socket,
                           std::vector<Client*> clients,
                           void(*handle_conns)(int socket, LEDTCPServer* server))
    : addr(addr),
      port(port),
      socket(socket),
      conn_info(new ClientConnInfo(clients)),
      conn_handling(NULL)
{}

void LEDTCPServer::start() {
    this->conn_handling = new std::thread(handle_conns, socket, this);
}

ClientConnInfo::ClientConnInfo(std::vector<Client *> clients)
    : mut(),
      connected(),
      disconnected()
{
    for (auto c : clients) {
        disconnected.insert(c);
    }
}

void ClientConnInfo::setConnected(const Client *c, int socket) {
    this->mut.lock();
    if (this->connected.find(c) == this->connected.end()) {
        this->disconnected.erase(c);
        this->connected[c] = socket;
    }
    this->mut.unlock();
}

std::optional<int> ClientConnInfo::getSocket(const Client *c) {
    this->mut.lock();
    auto conn = this->connected.find(c);
    this->mut.unlock();
    if (conn != this->connected.end()) {
        return conn->second;
    } else {
        return std::nullopt;
    }
}

void ClientConnInfo::getAllConnected(std::vector<std::pair<const Client*, int>>& v) {
    this->mut.lock();
    for (auto it : this->connected) {
        v.push_back(std::make_pair(it.first, it.second));
    }
    this->mut.unlock();
}

void ClientConnInfo::getAllDisconnected(std::vector<const Client*>& v) {
    this->mut.lock();
    for (auto c : this->disconnected) {
        v.push_back(c);
    }
    this->mut.unlock();
}

void ClientConnInfo::setDisconnected(const Client* c) {
    this->mut.lock();
    if (this->disconnected.find(c) == this->disconnected.end()) {
        this->connected.erase(c);
        this->disconnected.insert(c);
    }
    this->mut.unlock();
}

bool ClientConnInfo::isConnected(const Client *c) {
    this->mut.lock();
    return this->connected.find(c) != this->connected.end();
    this->mut.unlock();
}

void LEDTCPServer::tcp_send(const Client* c, int socket, void* data, int size) {
    int sent = send(socket, data, size, MSG_NOSIGNAL);
    if (sent != size) {
        std::cout << "Error sending set_leds: " << strerror(errno) << "\n";
        if (errno == ECONNRESET) {
            auto socket_opt = this->conn_info->getSocket(c);
            if (socket_opt.has_value()) {
                close(socket_opt.value());
            }
            this->conn_info->setDisconnected(c);
        }
    }
}

MessageHeader LEDTCPServer::tcp_recv_header(int socket) {
    MessageHeader header;
    struct pollfd pfd = {socket, POLLIN, 0};
    int total = 0;
    while (total < (int)sizeof(MessageHeader)) {
        poll(&pfd, 1, -1);
        int recved = recv(socket, (char*)&header + total, sizeof(header) - total, 0);
        if (recved < 0) {
            std::cerr << "Error receiving header: " << strerror(errno) << "\n";
        } else {
            total += recved;
        }
    }
    return header;
}

void LEDTCPServer::tcp_recv(int socket, void* data, int size) {
    struct pollfd pfd = {socket, POLLIN, 0};
    poll(&pfd, 1, -1);
    int total = 0;
    while (total < size) {
        std::cout << "test";
        poll(&pfd, 1, -1);
        int recved = recv(socket, (char*)data + total, size - total, 0);
        if (recved < 0) {
            std::cerr << "Error receiving: " << strerror(errno) << "\n";
        } else {
            total += recved;
        }
    }
}

void LEDTCPServer::set_leds(const Client* c, int client_socket, VirtualCanvas canvas, LEDMatrix* ledmat, uint8_t pin, uint8_t bit_depth) {
    uint32_t width = ledmat->spec->width;
    uint32_t height = ledmat->spec->height;
    // swap width and height if rotated +/-90 degrees
    rotation rot = ledmat->pos.rot;
    if (rot == LEFT || rot == RIGHT) {
        uint32_t temp = width;
        height = width;
        height = temp;
    }
    uint32_t x = ledmat->pos.x;
    uint32_t y = ledmat->pos.y;

    cv::Mat sub_cvmat = canvas.getPixelMatrix()(cv::Rect(x, y, width, height)).clone();
    if (rot == LEFT) {
        cv::rotate(sub_cvmat, sub_cvmat, cv::ROTATE_90_CLOCKWISE);
    } else if (rot == RIGHT) {
        cv::rotate(sub_cvmat, sub_cvmat, cv::ROTATE_90_COUNTERCLOCKWISE);
    } else if (rot == DOWN) {
        cv::rotate(sub_cvmat, sub_cvmat, cv::ROTATE_180);
    }

    uint32_t array_size = ledmat->packed_pixel_array_size;
    uint32_t msg_size;
    SetLedsMessage* msg_buf = encode_fixed_set_leds(pin, bit_depth, array_size, &msg_size);
    uint8_t* pixel_buf = &(msg_buf->pixel_data[0]);
    const uint8_t* data = sub_cvmat.data;
    const int brightness_reduction = 10;
    for (uint32_t i = 0; (i < ledmat->packed_pixel_array_size / 3); ++i) {
        uint32_t a = i * 3;
        if ((i / width) % 2 != 0) {
            pixel_buf[a + 2] = data[a] / brightness_reduction;
            pixel_buf[a + 1] = data[a + 1] / brightness_reduction;
            pixel_buf[a] = data[a + 2] / brightness_reduction;
        } else {
            uint32_t irem = i % width;
            uint32_t b = (((width - 1) - irem) + (i - irem)) * 3;
            pixel_buf[a + 2] = data[b] / brightness_reduction;
            pixel_buf[a + 1] = data[b + 1] / brightness_reduction;
            pixel_buf[a] = data[b + 2] / brightness_reduction;
        }
    }
    // uint32_t msg_size = ledmat->packed_pixel_array_size;
    this->tcp_send(c, client_socket, msg_buf, msg_size);
    free_message_buffer(msg_buf);
}
