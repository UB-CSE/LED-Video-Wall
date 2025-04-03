#include "config-parser.hpp"
#include "client.hpp"
#include <cstdint>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <yaml-cpp/exceptions.h>
#include <yaml-cpp/node/detail/iterator_fwd.h>
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <regex>
#include <opencv2/opencv.hpp>
#include <limits>

ServerConfig::ServerConfig(std::vector<Client*> clients,
                           cv::Size canvas_size,
                           int64_t ns_per_tick,
                           int64_t ns_per_frame)
    : clients(clients),
      canvas_size(canvas_size),
      ns_per_tick(ns_per_tick),
      ns_per_frame(ns_per_frame)
{}

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

YAML::Node yaml_key_present_and_unique(YAML::Node ynode, std::string key) {
    YAML::Node key_node = ynode[key];
    if (!key_node) {
        throw std::logic_error("Missing '" + key + "' key!");
    }
    
    bool present = false;
    for (auto it = ynode.begin(); it != ynode.end(); it++) {
        if (it->first.as<std::string>() == key) {
            if (present) {
                throw std::logic_error("Duplicates of '" + key + "' key!");
            } else {
                present = true;
            }
        }
    }

    return key_node;
}

std::map<std::string, LEDMatrixSpec*>
parse_matrix_specs(YAML::Node ynode_matrix_specs) {
    std::map<std::string, LEDMatrixSpec*> matrix_specs;
    for (YAML::const_iterator it=ynode_matrix_specs.begin(); it!=ynode_matrix_specs.end(); ++it) {
        std::string id = it->first.as<std::string>();

        if (matrix_specs.count(id) > 0) {
            std::stringstream ss;
            throw std::logic_error("Multiple matrix-specs named '" + id + "'!");
        }

        YAML::Node spec_node = it->second;

        float power_limit = yaml_key_present_and_unique(spec_node, "power_limit_amps").as<float>();
        YAML::Node width_height_node = yaml_key_present_and_unique(spec_node, "width-height");
        if (width_height_node.size() != 2) {
            throw std::logic_error("width-height key has wrong number of fields!");
        }
        uint32_t width = width_height_node[0].as<uint32_t>();
        uint32_t height = width_height_node[1].as<uint32_t>();

        matrix_specs[id] = new LEDMatrixSpec(id, power_limit, width, height);
    }
    return matrix_specs;
}

std::pair<std::map<std::string, LEDMatrix*>, cv::Size>
parse_matrices(YAML::Node ynode_matrices,
               std::map<std::string, LEDMatrixSpec*> matrix_specs) {
    std::map<std::string, LEDMatrix*> matrices;

    uint32_t min_x = std::numeric_limits<uint32_t>::max();
    uint32_t max_x = std::numeric_limits<uint32_t>::min();
    uint32_t min_y = std::numeric_limits<uint32_t>::max();
    uint32_t max_y = std::numeric_limits<uint32_t>::min();
    for (YAML::const_iterator it=ynode_matrices.begin(); it!=ynode_matrices.end(); ++it) {
        std::string id = it->first.as<std::string>();

        if (matrices.count(id) > 0) {
            std::stringstream ss;
            throw std::logic_error("Multiple matrices named '" + id + "'!");
        }

        YAML::Node matrix_node = it->second;

        std::string spec_id = yaml_key_present_and_unique(matrix_node, "spec").as<std::string>();
        
        YAML::Node pos_node = yaml_key_present_and_unique(matrix_node, "pos");
        if (pos_node.size() != 2) {
            throw YAML::RepresentationException(matrix_node["pos"].Mark(),
                                                "Position requires two values!");
        }
        YAML::Node rot_node = yaml_key_present_and_unique(matrix_node, "rot");
        std::optional<rotation> rot_opt = parse_rotation(rot_node.as<std::string>());
        if (!rot_opt.has_value()) {
            throw YAML::TypedBadConversion<rotation>(matrix_node["rot"].Mark());
        }
        rotation rot = rot_opt.value();
        LEDMatrixSpec* mat_spec = matrix_specs[spec_id];
        uint32_t width = mat_spec->width;
        uint32_t height = mat_spec->height;
        if (rot == LEFT || rot == RIGHT) {
            uint32_t temp = width;
            width = height;
            height = temp;
        }
        uint32_t x = pos_node[0].as<int>();
        uint32_t y = pos_node[1].as<int>();

        min_x = std::min(min_x, x);
        max_x = std::max(max_x, x + width);
        min_y = std::min(min_y, y);
        max_y = std::max(max_y, y + height);
        
        CanvasPos pos = CanvasPos(x,
                                  y,
                                  width,
                                  height,
                                  rot);
        LEDMatrix* mat = new LEDMatrix(id, mat_spec, pos);
        matrices[id] = mat;
    }

    // TODO: normalize positions after min values known?

    return std::make_pair(matrices, cv::Size(max_x, max_y));
}

bool overlaping_range(uint32_t i, uint32_t iwidth, uint32_t j, uint32_t jwidth) {
    uint32_t i2 = i + iwidth - 1;
    uint32_t j2 = j + jwidth - 1;
    return (i < j && i2 >= j) || (j2 >=  i);
}

void bounds_check_matrices(std::map<std::string, LEDMatrix *> matrices) {
    if (matrices.size() < 2) return;
    std::vector<LEDMatrix*> mat_vec;
    for (auto const& i : matrices) {
        mat_vec.push_back(i.second);
    }

    // TODO: find more efficient algo, current one is O(n^2) :(
    for (LEDMatrix* i : mat_vec) {
        for (LEDMatrix* j : mat_vec) {
            if (i == j) break;
            if (overlaping_range(i->pos.x, i->pos.width, j->pos.x, j->pos.width)) {
                if (overlaping_range(i->pos.y, i->pos.height, j->pos.y, j->pos.height)) {
                    std::stringstream ss;
                    ss << "Overlapping boundaries on matrices ";
                    ss << i->id << " and ";
                    ss << j->id << "\n";
                    throw std::logic_error(ss.str());
                }
            }
        }
    }
}

std::vector<Client*>
parse_clients(YAML::Node ynode_clients, std::map<std::string, LEDMatrix*> matrices) {
    std::vector<Client*> clients;

    for (YAML::const_iterator it=ynode_clients.begin(); it!=ynode_clients.end(); ++it) {
        std::string mac_addr_str = it->first.as<std::string>();
        std::optional<uint64_t> mac_addr_opt = parse_mac_addr_48_bit(mac_addr_str);
        if (!mac_addr_opt.has_value()) {
            throw YAML::RepresentationException(it->first.Mark(), "Invalid MAC Address!");
        }
        uint64_t mac_addr = mac_addr_opt.value();

        bool unique = std::find_if(clients.begin(),
                                   clients.end(),
                                   [mac_addr] (Client* c) {return c->mac_addr == mac_addr;})
            == clients.end();
        if (!unique) {
            std::stringstream ss;
            throw std::logic_error("Multiple clients with MAC address '" + mac_addr_str + "'!");
        }

        YAML::Node connections_node = yaml_key_present_and_unique(it->second, "matrix-connections");

        // Parse Matrix Connections
        std::vector<MatricesConnection> mat_connections;
        for (size_t i = 0; i < connections_node.size(); ++i) {
            YAML::Node connection_node = connections_node[i];
            uint8_t pin = connection_node["pin"].as<uint8_t>();
            MatricesConnection conn;
            conn.pin = pin;
            YAML::Node matrices_node = yaml_key_present_and_unique(connection_node, "matrices");
            for (size_t j = 0; j < matrices_node.size(); ++j) {
                std::string id = matrices_node[j].as<std::string>();
                conn.matrices.push_back(matrices[id]);
            }
            mat_connections.push_back(conn);
        }
        clients.push_back(new Client(mac_addr, mat_connections));
    }

    return clients;
}

ServerConfig parse_config_throws(std::string file) {
    YAML::Node config = YAML::LoadFile(file);

    YAML::Node ynode_clients = yaml_key_present_and_unique(config, "clients");
    YAML::Node ynode_matrices = yaml_key_present_and_unique(config, "matrices");
    YAML::Node ynode_matrix_specs = yaml_key_present_and_unique(config, "matrix-specs");
    YAML::Node ynode_ignore_bounds_checks = config["ignore-bounds-checks"];
    YAML::Node ynode_ns_per_tick = yaml_key_present_and_unique(config, "ns-per-tick");
    YAML::Node ynode_ns_per_frame = yaml_key_present_and_unique(config, "ns-per-frame");

    // Parse ns per tick/frame
    int64_t ns_per_tick = ynode_ns_per_tick.as<int64_t>();
    if (ns_per_tick <= 0) {
        throw YAML::RepresentationException(ynode_ns_per_tick.Mark(),
                                            "'ns-per-tick' must be non-zero and positive!");
    }
    int64_t ns_per_frame = ynode_ns_per_frame.as<int64_t>();
    if (ns_per_frame <= 0) {
        throw YAML::RepresentationException(ynode_ns_per_frame.Mark(),
                                            "'ns-per-frame' must be non-zero and positive!");
    }


    // Parse Matrix Specifications
    std::map<std::string, LEDMatrixSpec*> matrix_specs =
        parse_matrix_specs(ynode_matrix_specs);
    

    // Parse Matrices
    std::pair<std::map<std::string, LEDMatrix*>, cv::Size> matrices =
        parse_matrices(ynode_matrices, matrix_specs);

    // Do bounds checks
    if (!(ynode_ignore_bounds_checks && (ynode_ignore_bounds_checks.as<std::string>() == "true"))) {
        bounds_check_matrices(matrices.first);
    }

    // Parse Clients
    std::vector<Client*> clients =
        parse_clients(ynode_clients, matrices.first);

    return ServerConfig(clients, matrices.second, ns_per_tick, ns_per_frame);
}
