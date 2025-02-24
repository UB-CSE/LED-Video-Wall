#ifndef MATRIX_CONFIG_H
#define MATRIX_CONFIG_H

#include "controller_region.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <yaml-cpp/yaml.h>

// Forward declarations
class MatrixSpec;
class Matrix;
class Client;
class MatrixConnection;

// Represents a matrix specification (e.g., ws2812b:32x8)
class MatrixSpec {
public:
  float power_limit_amps;
  int width;
  int height;

  static MatrixSpec fromYAML(const YAML::Node &node) {
    MatrixSpec spec;
    spec.power_limit_amps = node["power_limit_amps"].as<float>();
    auto wh = node["width-height"].as<std::vector<int>>();
    spec.width = wh[0];
    spec.height = wh[1];
    return spec;
  }
};

// Represents a physical LED matrix with position and rotation
class Matrix {
public:
  enum class Rotation { UP, DOWN, LEFT, RIGHT };

  std::string name;
  MatrixSpec spec;
  cv::Point position;
  Rotation rotation;

  // Convert rotation string from config to enum
  static Rotation parseRotation(const std::string &rot) {
    if (rot == "up")
      return Rotation::UP;
    if (rot == "down")
      return Rotation::DOWN;
    if (rot == "left")
      return Rotation::LEFT;
    return Rotation::RIGHT;
  }

  // Get all pixel positions for this matrix based on its position and rotation
  std::vector<cv::Point> getPixelPositions() const {
    std::vector<cv::Point> pixels;
    pixels.reserve(spec.width * spec.height);

    for (int y = 0; y < spec.height; y++) {
      for (int x = 0; x < spec.width; x++) {
        cv::Point pixel;
        switch (rotation) {
        case Rotation::UP:
          pixel = cv::Point(x, y);
          break;
        case Rotation::DOWN:
          pixel = cv::Point(spec.width - 1 - x, spec.height - 1 - y);
          break;
        case Rotation::LEFT:
          pixel = cv::Point(y, spec.width - 1 - x);
          break;
        case Rotation::RIGHT:
          pixel = cv::Point(spec.height - 1 - y, x);
          break;
        }
        pixel += position;
        pixels.push_back(pixel);
      }
    }
    return pixels;
  }
};

// Represents a physical connection between a client and matrices
class MatrixConnection {
public:
  int pin;
  std::vector<std::string> matrix_names;

  static MatrixConnection fromYAML(const YAML::Node &node) {
    MatrixConnection conn;
    conn.pin = node["pin"].as<int>();
    conn.matrix_names = node["matrices"].as<std::vector<std::string>>();
    return conn;
  }
};

// Represents a client (microcontroller) with its MAC and connections
class Client {
public:
  std::string mac_address;
  std::vector<MatrixConnection> connections;

  static Client fromYAML(const std::string &mac, const YAML::Node &node) {
    Client client;
    client.mac_address = mac;
    for (const auto &conn : node["matrix-connections"]) {
      client.connections.push_back(MatrixConnection::fromYAML(conn));
    }
    return client;
  }
};

// Main configuration class that loads and manages the entire setup
class MatrixConfiguration {
private:
  std::unordered_map<std::string, MatrixSpec> specs;
  std::unordered_map<std::string, Matrix> matrices;
  std::unordered_map<std::string, Client> clients;

public:
  void loadFromFile(const std::string &filepath) {
    YAML::Node config = YAML::LoadFile(filepath);

    // Load matrix specs
    for (const auto &spec : config["matrix-specs"]) {
      std::string name = spec.first.as<std::string>();
      specs[name] = MatrixSpec::fromYAML(spec.second);
    }

    // Load matrices
    for (const auto &matrix : config["matrices"]) {
      std::string name = matrix.first.as<std::string>();
      Matrix mat;
      mat.name = name;
      mat.spec = specs[matrix.second["spec"].as<std::string>()];
      auto pos = matrix.second["pos"].as<std::vector<int>>();
      mat.position = cv::Point(pos[0], pos[1]);
      mat.rotation =
          Matrix::parseRotation(matrix.second["rot"].as<std::string>());
      matrices[name] = mat;
    }

    // Load clients
    for (const auto &client : config["clients"]) {
      std::string mac = client.first.as<std::string>();
      clients[mac] = Client::fromYAML(mac, client.second);
    }
  }

  // Build ControllerMapper from the configuration
  std::unique_ptr<ControllerMapper> buildMapper(const cv::Size &canvasSize) {
    auto mapper = std::make_unique<ControllerMapper>(canvasSize);

    // For each client
    for (const auto &[mac, client] : clients) {
      // For each pin connection
      for (const auto &conn : client.connections) {
        std::vector<cv::Point> regionPixels;

        // Collect all pixels from all matrices connected to this pin
        for (const auto &matrixName : conn.matrix_names) {
          const auto &matrix = matrices[matrixName];
          auto matrixPixels = matrix.getPixelPositions();
          regionPixels.insert(regionPixels.end(), matrixPixels.begin(),
                              matrixPixels.end());
        }

        // Create controller ID from MAC and pin
        std::string controllerId = mac + ":" + std::to_string(conn.pin);
        mapper->addRegion(std::hash<std::string>{}(controllerId), regionPixels);
      }
    }

    return mapper;
  }

  // Utility methods to access configuration data
  const auto &getSpecs() const { return specs; }
  const auto &getMatrices() const { return matrices; }
  const auto &getClients() const { return clients; }
};

#endif
