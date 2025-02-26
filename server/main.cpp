#include "canvas.h"
#include "matrix_config.h"
#include "web_viewer.h"
#include <chrono>
#include <cstdio>
#include <iostream>
#include <map>
#include <opencv2/opencv.hpp>
#include <random>
#include <thread>

// Function to generate a random bright color
cv::Vec3b generateRandomColor(std::mt19937 &rng) {
  // Generate bright, distinct colors (avoiding dark ones)
  std::uniform_int_distribution<int> distribution(100, 255);
  return cv::Vec3b(distribution(rng), // B
                   distribution(rng), // G
                   distribution(rng)  // R
  );
}

int main(int argc, char *argv[]) {
  if (argc <= 1) {
    std::cout << "Must specify matrix configuration path. \n"
              << "usage: canvas_test <config path>\n";
    return -1;
  }
  MatrixConfiguration config;
  try {
    config.loadFromFile(argv[1]);
    printf("Configuration loaded successfully\n");
  } catch (const std::exception &e) {
    printf("Failed to load configuration: %s\n", e.what());
    return 1;
  }

  // Calculate total canvas size based on matrix positions
  cv::Point maxPoint(0, 0);
  for (const auto &[name, matrix] : config.getMatrices()) {
    cv::Point matrixEnd = matrix.position;
    if (matrix.rotation == Matrix::Rotation::R90 ||
        matrix.rotation == Matrix::Rotation::R270) {
      matrixEnd += cv::Point(matrix.spec.height, matrix.spec.width);
    } else {
      matrixEnd += cv::Point(matrix.spec.width, matrix.spec.height);
    }
    maxPoint.x = std::max(maxPoint.x, matrixEnd.x);
    maxPoint.y = std::max(maxPoint.y, matrixEnd.y);
  }

  // If no configuration is loaded or the size is too small, use a default size
  if (maxPoint.x < 512 || maxPoint.y < 512) {
    maxPoint.x = std::max(maxPoint.x, 512);
    maxPoint.y = std::max(maxPoint.y, 512);
  }

  // Create virtual canvas with calculated dimensions
  VirtualCanvas vCanvas(cv::Size(maxPoint.x, maxPoint.y));
  printf("Created canvas with size %dx%d\n", maxPoint.x, maxPoint.y);

  // Create controller mapper from configuration
  auto mapper = config.buildMapper(vCanvas.getDimensions());
  printf("Controller mapper initialized with %zu regions\n",
         mapper->getRegions().size());

  std::random_device rd;
  std::mt19937 rng(rd());

  // Create a map to store colors for each controller ID
  std::map<int, cv::Vec3b> controllerColors;

  // Assign a unique color to each controller region
  for (const auto &region : mapper->getRegions()) {
    controllerColors[region.getId()] = generateRandomColor(rng);
  }

  // Create a visualization image
  cv::Mat visualizationImage = cv::Mat::zeros(vCanvas.getDimensions(), CV_8UC3);

  // Fill each region with its assigned color
  for (const auto &region : mapper->getRegions()) {
    const cv::Vec3b &color = controllerColors[region.getId()];
    for (const auto &pixel : region.getPixels()) {
      if (pixel.x >= 0 && pixel.x < visualizationImage.cols && pixel.y >= 0 &&
          pixel.y < visualizationImage.rows) {
        visualizationImage.at<cv::Vec3b>(pixel) = color;
      }
    }

    // Draw the region ID (controller ID) at the center of the region
    cv::Rect bbox = region.getBoundingBox();
    cv::Point center(bbox.x + bbox.width / 2, bbox.y + bbox.height / 2);

    // Set the visualization image to the canvas
    vCanvas.clear();
    visualizationImage.copyTo(vCanvas.getPixelMatrix());

    // Print debug information about regions
    printf("Region details:\n");
    for (const auto &region : mapper->getRegions()) {
      printf("Region ID: %d, Pixels: %zu, Bounding Box: (%d,%d,%d,%d)\n",
             region.getId(), region.getPixels().size(),
             region.getBoundingBox().x, region.getBoundingBox().y,
             region.getBoundingBox().width, region.getBoundingBox().height);
    }
  }

  // Create web viewer and start server
  WebViewer viewer;
  viewer.start(8080);
  printf("Web interface running at http://localhost:8080\n");

  // Main update loop
  while (true) {
    viewer.updateCanvas(vCanvas.getPixelMatrix());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return 0;
}
