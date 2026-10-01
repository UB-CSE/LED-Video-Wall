#pragma once

#include <yaml-cpp/yaml.h>
#include <opencv2/opencv.hpp>

namespace YAML {

// encode/decode cv::Point types
template <typename T> struct convert<cv::Point_<T>> {
  static Node encode(const cv::Point_<T> &point) {
    Node node = { point.x, point.y };
    return node;
  }

  static bool decode(const Node &node, cv::Point_<T> &point) {
    auto components = node.as<std::vector<T>>();
    if (components.size() != 2) {
      return false;
    }

    point.x = components[0];
    point.y = components[1];

    return true;
  }
};

// encode/decode cv::Size types
template <typename T> struct convert<cv::Size_<T>> {
  static Node encode(const cv::Size_<T> &size) {
    Node node;
    node["width"] = size.width;
    node["height"] = size.height;
    return node;
  }

  static bool decode(const Node &node, cv::Size_<T> &size) {
    if (!node.IsMap() || !node["width"] || !node["height"]) {
      return false;
    }

    size.width = node["width"].as<T>();
    size.height = node["height"].as<T>();

    return true;
  }
};

} // namespace YAML
