#pragma once

#include <opencv2/opencv.hpp>
#include <yaml-cpp/yaml.h>

namespace YAML {

// encode/decode cv::Point types
template <typename T> struct convert<cv::Point_<T>> {
  static Node encode(const cv::Point_<T> &point) {
    Node node(NodeType::Sequence);
    node.push_back(point.x);
    node.push_back(point.y);
    return node;
  }

  static bool decode(const Node &node, cv::Point_<T> &point) {
    if (!node.IsSequence() || node.size() != 2) {
      return false;
    }

    point.x = node[0].as<T>();
    point.y = node[1].as<T>();
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
