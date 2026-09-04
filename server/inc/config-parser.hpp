#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include "client.hpp"
#include <cstdint>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

class ServerConfig {
public:
  std::vector<Client *> clients;
  cv::Size canvas_size;
  int64_t ns_per_frame;
  float brightness_percent;

  ServerConfig();

  ServerConfig(std::vector<Client *> clients, cv::Size canvas_size,
               int64_t ns_per_frame, float brightness_percent);
};

ServerConfig parse_config_throws(std::string file);

#endif
