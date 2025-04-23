#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <cstdint>
#include <string>
#include <vector>
#include "client.hpp"
#include <opencv2/opencv.hpp>

class ServerConfig {
public:
    std::vector<Client*> clients;
    cv::Size canvas_size;
    int64_t ns_per_frame;

    ServerConfig();

    ServerConfig(std::vector<Client*> clients,
                 cv::Size canvas_size,
                 int64_t ns_per_frame);
};

ServerConfig parse_config_throws(std::string file);

#endif
