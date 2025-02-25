#include "client.hpp"
#include <cstdint>
#include <regex>
#include <optional>
#include <string>
#include <ios>

std::regex rot_regex("^(up|down|left|right)$");

rotation parse_rotation(std::string str) {
    std::smatch match;
    std::regex_search(str, match, rot_regex);
    std::string str_match = match[0].str();
    if (str_match == "up") {
        return UP;
    } else if (str_match == "down") {
        return DOWN;
    } else if (str_match == "left") {
        return LEFT;
    } else if (str_match == "right") {
        return RIGHT;
    }

    //todo error
}

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
    ss << "height: " << this->width << ",";
    ss << "total_leds: " << this->total_leds << "]";
    return ss.str();
}

LEDMatrix::LEDMatrix(std::string id,
                     LEDMatrixSpec* spec,
                     CanvasPos pos):
    id(id),
    spec(spec),
    pos(pos)
{}

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
