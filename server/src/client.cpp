#include "client.hpp"
#include "tcp.hpp"
#include "msg.hpp"

#include <cstdint>
#include <regex>
#include <optional>
#include <string>
#include <ios>

// todo, make configurable later
const int NUM_CHANNELS = 3;
const int BIT_DEPTH = 8;

CanvasPos::CanvasPos(uint32_t x,
                     uint32_t y,
                     rotation rot):
    x(x), y(y), rot(rot)
{}

LEDMatrixSpec::LEDMatrixSpec(std::string id,
                             float power_limit_amps,
                             uint32_t width,
                             uint32_t height):
    id(id),
    power_limit_amps(power_limit_amps),
    width(width),
    height(height),
    total_leds(width * height)
{}

std::string LEDMatrixSpec::to_string() {
    std::stringstream ss;
    ss << "LEDMatrixSpec[";
    ss << "id: " << this->id << ", ";
    ss << "power_limit_amps: " << this->power_limit_amps << ", ";
    ss << "width: " << this->width << ",";
    ss << "height: " << this->height << ",";
    ss << "total_leds: " << this->total_leds << "]";
    return ss.str();
}

LEDMatrix::LEDMatrix(std::string id,
                     LEDMatrixSpec *spec,
                     CanvasPos pos,
                     msg_size_t msg_size,
                     MSGBUF_Send_set_leds* msg)
    : id(id),
      spec(spec),
      pos(pos),
      msg_size(msg_size),
      msg(msg)
{
    
    this->packed_pixel_array_size = spec->total_leds * NUM_CHANNELS;
}

std::string LEDMatrix::to_string() {
    std::stringstream ss;
    ss << "LEDMatrix[";
    ss << "spec: " << this->spec->to_string() << ", ";
    ss << "pos: " << "(" << this->pos.x << ", " << this->pos.y << ")" << "]";
    return ss.str();
}

std::string MatricesConnection::to_string() {
    std::stringstream ss;
    ss << "MatricesConnection[";
    ss << "pin: " << std::to_string(this->pin) << ", ";
    ss << "matrices: (";
    for (LEDMatrix* mat : this->matrices) {
        ss << mat->to_string() << ", ";
    }
    ss << ")]";
    return ss.str();
}

Client::Client(uint64_t mac_addr,
               int socket,
               std::vector<MatricesConnection> mat_connections):
    mac_addr(mac_addr),
    socket(socket),
    mat_connections(mat_connections)
{}

std::string Client::to_string() {
    std::stringstream ss;
    ss << "Client[";
    ss << "mac-addr: " << std::hex << this->mac_addr << ", ";
    ss << "socket: " << std::to_string(this->socket) << ", ";
    ss << "mat_connections: (";
    for (MatricesConnection conn : this->mat_connections) {
        ss << conn.to_string() << ", ";
    }
    ss << ")]";
    return ss.str();
}

void Client::set_leds_all_matrices(const cv::Mat &cvmat) {
    for (MatricesConnection conn : this->mat_connections) {
        uint8_t pin = conn.pin;
        for (LEDMatrix* mat : conn.matrices) {
            tcp_set_leds(this->socket, cvmat, mat, pin, BIT_DEPTH);
        }
    }
}
