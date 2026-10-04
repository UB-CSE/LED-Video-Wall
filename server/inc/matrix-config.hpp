#pragma once

#include "client.hpp"
#include "protocol.hpp"
#include <cstdint>
#include <filesystem>
#include <memory>
#include <opencv2/opencv.hpp>
#include <vector>

struct MatrixConfig {
  int ledvwPort;
  int rtmpPort;
  std::vector<std::shared_ptr<Client>> clients;
  cv::Size canvas_size;
  int64_t ns_per_frame;
  float brightness_percent;
  ImageEncoding image_encoding;

  /**
   * Load a matrix configuration YAML file.
   *
   * @param filepath Path to config file.
   * @return True if successful, false otherwise.
   */
  bool load(const std::filesystem::path &filepath);
};