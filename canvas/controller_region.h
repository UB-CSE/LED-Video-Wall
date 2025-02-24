#ifndef CONTROLLER_REGION_H
#define CONTROLLER_REGION_H

#include <opencv2/opencv.hpp>
#include <unordered_map>
#include <vector>

// Represents a region controlled by a single microcontroller
class ControllerRegion {
private:
  int controllerId;
  std::vector<cv::Point> pixels; // List of pixels this controller manages
  cv::Rect boundingBox;          // Bounding box for quick intersection tests

public:
  ControllerRegion(int id, const std::vector<cv::Point> &pixelPoints)
      : controllerId(id), pixels(pixelPoints) {
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
      cv::Vec3b pixel = canvas.at<cv::Vec3b>(point);
      packedData.push_back(pixel[2]); // R
      packedData.push_back(pixel[1]); // G
      packedData.push_back(pixel[0]); // B
    }

    return packedData;
  }

  int getId() const { return controllerId; }
  const cv::Rect &getBoundingBox() const { return boundingBox; }
  const std::vector<cv::Point> &getPixels() const { return pixels; }
};

// Manages all controller regions and their mapping to the canvas
class ControllerMapper {
private:
  std::vector<ControllerRegion> regions;
  cv::Size canvasSize;

public:
  ControllerMapper(const cv::Size &size) : canvasSize(size) {}

  // Add a new controller region
  void addRegion(int controllerId, const std::vector<cv::Point> &pixels) {
    regions.emplace_back(controllerId, pixels);
  }

  // Define a region using a mask image (non-zero pixels define the region)
  void addRegionFromMask(int controllerId, const cv::Mat &mask) {
    std::vector<cv::Point> pixels;
    for (int y = 0; y < mask.rows; y++) {
      for (int x = 0; x < mask.cols; x++) {
        if (mask.at<uint8_t>(y, x) > 0) {
          pixels.emplace_back(x, y);
        }
      }
    }
    addRegion(controllerId, pixels);
  }

  // Get packed data for affected controllers when an element is added/modified
  std::vector<std::pair<int, std::vector<uint8_t>>>
  getAffectedRegions(const Element &element, const cv::Mat &canvas) {

    std::vector<std::pair<int, std::vector<uint8_t>>> updates;
    cv::Rect elementRect(element.getLocation(), element.getDimensions());

    // Check each region for intersection with the element
    for (const auto &region : regions) {
      if (elementRect.contains(region.getBoundingBox()) ||
          region.getBoundingBox().contains(elementRect)) {

        updates.emplace_back(region.getId(), region.getPackedPixels(canvas));
      }
    }

    return updates;
  }
};

#endif
