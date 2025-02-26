#ifndef CONTROLLER_REGION_H
#define CONTROLLER_REGION_H

#include <opencv2/opencv.hpp>
#include <unordered_map>
#include <vector>

// Represents a region controlled by a single microcontroller (e.g. all the
// led matrix panels that are associated)
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
  // we'll probably need to rewrite..
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

  // Get packed data for affected controllers when an element is added/modified
  // This just checks the bounds the element and sees which microcontroller
  // regions need to be updated.
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

  // Get all regions (needed for debugging visualization)
  const std::vector<ControllerRegion> &getRegions() const { return regions; }
};

#endif
