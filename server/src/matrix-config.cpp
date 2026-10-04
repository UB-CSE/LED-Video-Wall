#include "matrix-config.hpp"
#include "client.hpp"
#include "encoding-util.hpp"
#include <fstream>
#include <iostream>
#include <limits>
#include <opencv2/opencv.hpp>
#include <ranges>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>
#include <unordered_map>

struct MACAddress {
  uint64_t mac;
};

namespace YAML {

template <> struct convert<Rotation> {
  static bool decode(const Node &node, Rotation &rot) {
    const auto rotString = node.as<std::string>();
    rot = RotationFromString(rotString);
    const bool success = rot != Rotation::UNKNOWN;
    return success;
  }
};

template <> struct convert<ImageEncoding> {
  static bool decode(const Node &node, ImageEncoding &encoding) {
    const auto encString = node.as<std::string>();
    encoding = encoding_from_string(encString);
    const bool success = encoding != ImageEncoding::UNKNOWN;
    return success;
  }
};

template <> struct convert<MACAddress> {
  static uint8_t parseHex(char c) {
    if (c <= '9') {
      return c - '0';
    } else {
      return c - 'A' + 10;
    }
  }

  static inline std::regex mac_48_regex{
      "^[0-9A-F][0-9A-F](-[0-9A-F][0-9A-F]){5}$"};

  static bool decode(const Node &node, MACAddress &macAddress) {
    const auto addrString = node.as<std::string>();
    std::smatch match;
    std::regex_search(addrString, match, mac_48_regex);
    if (match.empty()) {
      return false;
    }
    uint64_t res = 0;
    for (int i = 5; i >= 0; --i) {
      int offset = i * 3;
      res <<= 4;
      res += parseHex(addrString[offset]);
      res <<= 4;
      res += parseHex(addrString[offset + 1]);
    }
    macAddress.mac = res;
    return true;
  }
};

} // namespace YAML

namespace {

using MatrixSpecsMap =
    std::unordered_map<std::string, std::shared_ptr<LEDMatrixSpec>>;

using MatricesMap = std::unordered_map<std::string, std::shared_ptr<LEDMatrix>>;

MatrixSpecsMap matrixSpecsFromYAML(YAML::Node node) {
  MatrixSpecsMap matrices;

  for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
    const auto id = it->first.as<std::string>();
    YAML::Node matrixSpecNode = it->second;

    const auto powerLimitAmps = matrixSpecNode["power-limit-amps"].as<float>();
    const auto dimensions = matrixSpecNode["dimensions"].as<cv::Size>();

    auto matrix = std::make_shared<LEDMatrixSpec>();
    matrix->id = id;
    matrix->powerLimitAmps = powerLimitAmps;
    matrix->width = dimensions.width;
    matrix->height = dimensions.height;
    matrices[id] = matrix;
  }

  return matrices;
}

std::pair<MatricesMap, cv::Size> matricesFromYAML(YAML::Node node,
                                                  MatrixSpecsMap &matrixSpecs) {
  MatricesMap matrices;

  uint32_t min_x = std::numeric_limits<uint32_t>::max();
  uint32_t max_x = std::numeric_limits<uint32_t>::min();
  uint32_t min_y = std::numeric_limits<uint32_t>::max();
  uint32_t max_y = std::numeric_limits<uint32_t>::min();

  for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
    const auto id = it->first.as<std::string>();
    YAML::Node matrixNode = it->second;

    const auto specID = matrixNode["spec"].as<std::string>();
    const auto position = matrixNode["pos"].as<cv::Point>();
    const auto rotation = matrixNode["rot"].as<Rotation>();

    std::shared_ptr<LEDMatrixSpec> spec = matrixSpecs[specID];
    if (!spec) {
      throw std::runtime_error("no such matrix spec defined: '" + specID + "'");
    }

    uint32_t width = spec->width;
    uint32_t height = spec->height;
    if (rotation == Rotation::LEFT || rotation == Rotation::RIGHT) {
      std::swap(width, height);
    }
    uint32_t x = position.x;
    uint32_t y = position.y;

    min_x = std::min(min_x, x);
    max_x = std::max(max_x, x + width);
    min_y = std::min(min_y, y);
    max_y = std::max(max_y, y + height);

    const CanvasPos canvasPos{
        .x = x, .y = y, .width = width, .height = height, .rot = rotation};

    auto matrix = std::make_shared<LEDMatrix>();
    matrix->id = id;
    matrix->spec = spec;
    matrix->pos = canvasPos;
    matrices[id] = matrix;
  }

  // TODO: normalize positions after min values known?

  return std::make_pair(matrices, cv::Size(max_x, max_y));
}

std::vector<std::shared_ptr<Client>> clientsFromYAML(YAML::Node node,
                                                     MatricesMap &matrices) {
  std::vector<std::shared_ptr<Client>> clients;

  for (YAML::const_iterator it = node.begin(); it != node.end(); ++it) {
    const uint64_t macAddr = it->first.as<MACAddress>().mac;
    YAML::Node clientNode = it->second;
    YAML::Node connectionsNode = clientNode["matrix-connections"];

    // Parse Matrix Connections
    std::vector<MatricesConnection> matConnections;
    for (size_t i = 0; i < connectionsNode.size(); ++i) {
      YAML::Node connectionNode = connectionsNode[i];
      int pin_raw = connectionNode["pin"].as<int>();
      auto pin = static_cast<int8_t>(pin_raw);

      MatricesConnection conn;
      conn.pin = pin;
      YAML::Node matricesNode = connectionNode["matrices"];

      for (size_t j = 0; j < matricesNode.size(); ++j) {
        const auto matrixID = matricesNode[j].as<std::string>();
        conn.matrices.push_back(matrices[matrixID]);
      }
      matConnections.push_back(conn);
    }

    auto client = std::make_shared<Client>();
    client->macAddr = macAddr;
    client->matConnections = matConnections;
    clients.push_back(client);
  }

  return clients;
}

bool isOverlappingRange(uint32_t i, uint32_t iWidth, uint32_t j,
                        uint32_t jWidth) {
  uint32_t i2 = i + iWidth - 1;
  uint32_t j2 = j + jWidth - 1;
  return (i < j && i2 >= j) || (j2 >= i);
}

void boundsCheckMatrices(const MatricesMap &matrices) {
  if (matrices.size() < 2)
    return;
  std::vector<std::shared_ptr<LEDMatrix>> mat_vec;
  for (const auto &mat : matrices | std::views::values) {
    mat_vec.push_back(mat);
  }

  // TODO: find more efficient algo, current one is O(n^2) :(
  for (const std::shared_ptr<LEDMatrix> &i : mat_vec) {
    for (const std::shared_ptr<LEDMatrix> &j : mat_vec) {
      if (i == j)
        break;
      if (isOverlappingRange(i->pos.x, i->pos.width, j->pos.x, j->pos.width)) {
        if (isOverlappingRange(i->pos.y, i->pos.height, j->pos.y,
                               j->pos.height)) {
          std::stringstream ss;
          ss << "Overlapping boundaries on matrices ";
          ss << i->id << " and ";
          ss << j->id << "\n";
          throw std::runtime_error(ss.str());
        }
      }
    }
  }
}

void validatePort(uint16_t port) {
  if (port < 1024) {
    throw std::runtime_error("system ports (0-1023) are reserved");
  }
}

} // namespace

bool MatrixConfig::load(const std::filesystem::path &filepath) {
  try {
    YAML::Node config = YAML::LoadFile(filepath);

    YAML::Node matrixSpecsNode = config["matrix-specs"];
    YAML::Node matricesNode = config["matrices"];
    YAML::Node clientsNode = config["clients"];
    YAML::Node settingsNode = config["settings"];

    // Settings

    bool ignoreBoundsChecks = settingsNode["ignore-bounds-checks"].as<std::string>() == "true";

    ns_per_frame = settingsNode["ns-per-frame"].as<int64_t>();
    if (ns_per_frame <= 0) {
      throw std::runtime_error("invalid ns-per-frame");
    }

    YAML::Node brightnessPercentNode = settingsNode["brightness-percent"];
    brightness_percent = 100.f;
    if (brightnessPercentNode) {
      brightness_percent = settingsNode["brightness-percent"].as<float>();
    }

    image_encoding = settingsNode["image-encoding"].as<ImageEncoding>();

    ledvwPort = settingsNode["ledvw-port"].as<uint16_t>();
    validatePort(ledvwPort);
    rtmpPort = settingsNode["rtmp-port"].as<uint16_t>();
    validatePort(rtmpPort);

    // Matrices

    MatrixSpecsMap matrixSpecs = matrixSpecsFromYAML(matrixSpecsNode);
    auto [matrices, matricesSize] = matricesFromYAML(matricesNode, matrixSpecs);

    // Clients

    clients = clientsFromYAML(clientsNode, matrices);
    canvas_size = matricesSize;

    if (!ignoreBoundsChecks) {
      boundsCheckMatrices(matrices);
    }

  } catch (const std::exception &e) {
    std::cerr << "failed to load matrix config: " << e.what() << std::endl;
    return false;
  }

  return true;
}
