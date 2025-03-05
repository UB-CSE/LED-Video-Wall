#include "client.hpp"
#include "msg.hpp"

#include <cstdint>
#include <optional>
#include <yaml-cpp/exceptions.h>
#include <yaml-cpp/node/detail/iterator_fwd.h>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <regex>

std::string parse_error(std::string error) {
    return "Parse Error: " + error;
}

std::regex rot_regex("^(up|down|left|right)$");

std::optional<rotation> parse_rotation(std::string str) {
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

    return std::nullopt;
}

std::regex mac_48_regex("^[0-9A-F][0-9A-F](-[0-9A-F][0-9A-F]){5}$");

uint8_t parse_hex(char c) {
    if (c <= '9') {
        return c - '0';
    } else {
        return c - 'A' + 10;
    }
}

std::optional<uint64_t> parse_mac_addr_48_bit(std::string str) {
    std::smatch match;
    std::regex_search(str, match, mac_48_regex);
    if (match.empty()) {
        return std::nullopt;
    }
    uint64_t res = 0;
    for (int i = 5; i >= 0; --i) {
        int offset = i * 3;
        res <<= 4;
        res += parse_hex(str[offset]);
        res <<= 4;
        res += parse_hex(str[offset + 1]);
    }
    return res;
}

std::vector<Client*> parse_config_throws(std::string file) {
    YAML::Node config = YAML::LoadFile(file);

    YAML::Node ynode_clients = config["clients"];
    YAML::Node ynode_matrices = config["matrices"];
    YAML::Node ynode_matrix_specs = config["matrix-specs"];

    // Parse Matrix Specifications
    std::map<std::string, LEDMatrixSpec*> matrix_specs;
    for (YAML::const_iterator it=ynode_matrix_specs.begin(); it!=ynode_matrix_specs.end(); ++it) {
        std::string id = it->first.as<std::string>();
        YAML::Node spec_node = it->second;

        float power_limit = spec_node["power_limit_amps"].as<float>();
        uint32_t width = spec_node["width-height"][0].as<uint32_t>();
        uint32_t height = spec_node["width-height"][1].as<uint32_t>();

        matrix_specs[id] = new LEDMatrixSpec(id, power_limit, width, height);
    }

    // Parse Matrices
    std::map<std::string, LEDMatrix*> matrices;
    // std::vector<LEDMatrix> matrices;
    for (YAML::const_iterator it=ynode_matrices.begin(); it!=ynode_matrices.end(); ++it) {
        std::string id = it->first.as<std::string>();
        YAML::Node matrix_node = it->second;

        std::string spec_id = matrix_node["spec"].as<std::string>();
        
        YAML::Node pos_node = matrix_node["pos"];
        if (pos_node.size() != 2) {
            throw YAML::RepresentationException(matrix_node["pos"].Mark(),
                                                "Position requires two values!");
        }
        std::optional<rotation> rot_opt = parse_rotation(matrix_node["rot"].as<std::string>());
        if (!rot_opt.has_value()) {
            throw YAML::TypedBadConversion<rotation>(matrix_node["rot"].Mark());
        }
        rotation rot = rot_opt.value();
        CanvasPos pos = CanvasPos(pos_node[0].as<int>(),
                                  pos_node[1].as<int>(),
                                  rot);
        LEDMatrixSpec* mat_spec = matrix_specs[spec_id];
        uint32_t msg_buf_size =
            MSGBUF_Send_set_leds::msg_size(mat_spec->total_leds, 1, 3);
        MSGBUF_Send_set_leds* msg =
            MSGBUF_Send_set_leds::make(mat_spec->total_leds, 1, 3);
        LEDMatrix* mat = new LEDMatrix(id, mat_spec, pos, msg_buf_size, msg);
        matrices[id] = mat;
    }

    // Parse Clients
    std::vector<Client*> clients;
    for (YAML::const_iterator it=ynode_clients.begin(); it!=ynode_clients.end(); ++it) {
        std::optional<uint64_t> mac_addr_opt = parse_mac_addr_48_bit(it->first.as<std::string>());
        if (!mac_addr_opt.has_value()) {
            throw YAML::RepresentationException(it->first.Mark(), "Invalid MAC Address!");
        }
        uint64_t mac_addr = mac_addr_opt.value();

        YAML::Node connections_node = it->second["matrix-connections"];

        // Parse Matrix Connections
        std::vector<MatricesConnection> mat_connections;
        for (size_t i = 0; i < connections_node.size(); ++i) {
            YAML::Node connection_node = connections_node[i];
            uint8_t pin = connection_node["pin"].as<uint8_t>();
            MatricesConnection conn;
            conn.pin = pin;
            YAML::Node matrices_node = connection_node["matrices"];
            for (size_t j = 0; j < matrices_node.size(); ++j) {
                std::string id = matrices_node[j].as<std::string>();
                conn.matrices.push_back(matrices[id]);
            }
            mat_connections.push_back(conn);
        }

        clients.push_back(new Client(mac_addr, -1, mat_connections));
    }

    return clients;
}
