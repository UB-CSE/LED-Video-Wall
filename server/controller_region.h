#ifndef CONTROLLER_REGION_H
#define CONTROLLER_REGION_H

#include "client.hpp"
#include "config-parser.hpp"
#include <algorithm>
#include <climits>
#include <opencv2/opencv.hpp>
#include <unordered_map>
#include <vector>

// Forward declaration of Element class from canvas.h
class Element;

// Represents a region controlled by a single microcontroller (e.g. all the
// led matrix panels that are associated)
class ControllerRegion {
private:
  int controllerId;
  std::vector<cv::Point> pixels; // List of pixels this controller manages
  cv::Rect boundingBox;          // Bounding box for quick intersection tests
  Client *client;    // Associated client for this region (can be nullptr)
  LEDMatrix *matrix; // Associated LED matrix for this region (can be nullptr)

public:
  ControllerRegion(int id, const std::vector<cv::Point> &pixelPoints)
      : controllerId(id), pixels(pixelPoints), client(nullptr),
        matrix(nullptr) {
    // Calculate bounding box for optimization
    int minX = INT_MAX, minY = INT_MAX;
    int maxX = INT_MIN, maxY = INT_MIN;

    for (const auto &point : pixels) {
      minX = std::min(minX, point.x);
      minY = std::min(minY, point.y);
      maxX = std::max(maxX, point.x);
      maxY = std::max(maxY, point.y);
    }

    boundingBox = cv::Rect(minX, minY, maxX - minX + 1, maxY - minY + 1);
  }

  // Constructor that also associates a client and matrix
  ControllerRegion(int id, const std::vector<cv::Point> &pixelPoints,
                   Client *client, LEDMatrix *matrix)
      : controllerId(id), pixels(pixelPoints), client(client), matrix(matrix) {
    // Calculate bounding box for optimization
    int minX = INT_MAX, minY = INT_MAX;
    int maxX = INT_MIN, maxY = INT_MIN;

    for (const auto &point : pixels) {
      minX = std::min(minX, point.x);
      minY = std::min(minY, point.y);
      maxX = std::max(maxX, point.x);
      maxY = std::max(maxY, point.y);
    }

    boundingBox = cv::Rect(minX, minY, maxX - minX + 1, maxY - minY + 1);
  }

  // Get packed pixel data for this region from the canvas
  std::vector<uint8_t> getPackedPixels(const cv::Mat &canvas) const {
    std::vector<uint8_t> packedData;
    packedData.reserve(pixels.size() * 3); // RGB for each pixel

    for (const auto &point : pixels) {
      if (point.y < canvas.rows && point.x < canvas.cols) {
        cv::Vec3b pixel = canvas.at<cv::Vec3b>(point);
        packedData.push_back(pixel[2]); // R
        packedData.push_back(pixel[1]); // G
        packedData.push_back(pixel[0]); // B
      }
    }

    return packedData;
  }

  int getId() const { return controllerId; }
  const cv::Rect &getBoundingBox() const { return boundingBox; }
  const std::vector<cv::Point> &getPixels() const { return pixels; }
  Client *getClient() const { return client; }
  LEDMatrix *getMatrix() const { return matrix; }
};

// Manages all controller regions and their mapping to the canvas
class ControllerMapper {
private:
  std::vector<ControllerRegion> regions;
  cv::Size canvasSize;
  std::vector<Client> clients; // Store parsed clients from config

public:
  ControllerMapper(const cv::Size &size) : canvasSize(size) {}

  // Constructor that also takes a config file path
  ControllerMapper(const cv::Size &size, const std::string &configPath)
      : canvasSize(size) {
    loadFromConfig(configPath);
  }

  // Add a new controller region
  void addRegion(int controllerId, const std::vector<cv::Point> &pixels) {
    regions.emplace_back(controllerId, pixels);
  }

  // Add a new controller region with client and matrix information
  void addRegion(int controllerId, const std::vector<cv::Point> &pixels,
                 Client *client, LEDMatrix *matrix) {
    regions.emplace_back(controllerId, pixels, client, matrix);
  }

  // Load controller regions from a configuration file
  void loadFromConfig(const std::string &configPath) {
    try {
      // Parse config file
      clients = parse_config_throws(configPath);

      // Create regions for each matrix in each client
      int regionId = 0;
      for (auto &client : clients) {
        for (auto &connection : client.mat_connections) {
          for (auto *matrix : connection.matrices) {
            // Create pixel list for this matrix
            std::vector<cv::Point> pixels;

            uint32_t width = matrix->spec->width;
            uint32_t height = matrix->spec->height;
            uint32_t startX = matrix->pos.x;
            uint32_t startY = matrix->pos.y;

            // Apply rotation if needed
            bool swapWidthHeight =
                (matrix->pos.rot == LEFT || matrix->pos.rot == RIGHT);

            for (uint32_t y = 0; y < (swapWidthHeight ? width : height); y++) {
              for (uint32_t x = 0; x < (swapWidthHeight ? height : width);
                   x++) {
                cv::Point pixel;

                // Apply rotation transformations
                switch (matrix->pos.rot) {
                case UP:
                  pixel = cv::Point(startX + x, startY + y);
                  break;
                case DOWN:
                  pixel = cv::Point(startX + width - 1 - x,
                                    startY + height - 1 - y);
                  break;
                case LEFT:
                  pixel = cv::Point(startX + y, startY + height - 1 - x);
                  break;
                case RIGHT:
                  pixel = cv::Point(startX + width - 1 - y, startY + x);
                  break;
                }

                pixels.push_back(pixel);
              }
            }

            // Store a pointer to the actual client (not a copy)
            Client *clientPtr = &clients.back();
            for (int i = 0; i < clients.size() - 1; i++) {
              if (clients[i].mac_addr == client.mac_addr) {
                clientPtr = &clients[i];
                break;
              }
            }

            // Add region with client and matrix information
            regions.emplace_back(regionId++, pixels, clientPtr, matrix);
          }
        }
      }

    } catch (const std::exception &e) {
      std::cerr << "Failed to load configuration: " << e.what() << std::endl;
      throw;
    }
  }

  // Get packed data for affected controllers when an element is added/modified
  std::vector<std::pair<int, std::vector<uint8_t>>>
  getAffectedRegions(const Element &element, const cv::Mat &canvas) {
    std::vector<std::pair<int, std::vector<uint8_t>>> updates;
    cv::Rect elementRect(element.getLocation(), element.getDimensions());

    // Check each region for intersection with the element
    for (const auto &region : regions) {
      if ((elementRect & region.getBoundingBox()).area() > 0) {
        updates.emplace_back(region.getId(), region.getPackedPixels(canvas));
      }
    }

    return updates;
  }

  // Get client-specific updates when an element is added/modified
  std::vector<std::pair<Client *, std::vector<uint8_t>>>
  getClientUpdates(const Element &element, const cv::Mat &canvas) {
    std::vector<std::pair<Client *, std::vector<uint8_t>>> clientUpdates;
    cv::Rect elementRect(element.getLocation(), element.getDimensions());

    // Group regions by client
    std::unordered_map<Client *, std::vector<const ControllerRegion *>>
        clientRegions;

    for (const auto &region : regions) {
      if ((elementRect & region.getBoundingBox()).area() > 0 &&
          region.getClient() != nullptr) {
        Client *client = region.getClient();
        clientRegions[client].push_back(&region);
      }
    }

    // Combine pixel data for each client
    for (const auto &[client, affectedRegions] : clientRegions) {
      std::vector<uint8_t> combinedData;

      for (const auto &region : affectedRegions) {
        auto packedPixels = region->getPackedPixels(canvas);
        combinedData.insert(combinedData.end(), packedPixels.begin(),
                            packedPixels.end());
      }

      clientUpdates.emplace_back(client, combinedData);
    }

    return clientUpdates;
  }

  // Get all regions (needed for debugging visualization)
  const std::vector<ControllerRegion> &getRegions() const { return regions; }

  // Get access to all clients
  const std::vector<Client> &getClients() const { return clients; }
};

#endif
