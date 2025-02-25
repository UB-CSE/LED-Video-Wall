#include "canvas.h"
#include "matrix_config.h"
#include "web_viewer.h"
#include <chrono>
#include <opencv2/opencv.hpp>
#include <thread>

int main() {
  // Initialize configuration
  MatrixConfiguration config;
  try {
    config.loadFromFile("matrix_config.yaml");
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

  // Create virtual canvas with calculated dimensions
  VirtualCanvas vCanvas(cv::Size(maxPoint.x, maxPoint.y));
  printf("Created canvas with size %dx%d\n", maxPoint.x, maxPoint.y);

  // Create controller mapper from configuration
  auto mapper = config.buildMapper(vCanvas.getDimensions());
  printf("Controller mapper initialized\n");

  // Create web viewer and start server
  WebViewer viewer;
  viewer.start(8080);
  printf("Web interface running at http://localhost:8080\n");

  // Main update loop
  while (true) {
    viewer.updateCanvas(vCanvas.getPixelMatrix());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if (cv::waitKey(1) == 27) { // ESC key
      break;
    }
  }

  return 0;
}
