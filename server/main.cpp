#include "canvas.h"
#include "web_viewer.h"
#include <chrono>
#include <opencv2/opencv.hpp>
#include <thread>

int main() {
  VirtualCanvas vCanvas(cv::Size(512, 512));

  // Create web viewer and start server
  WebViewer viewer;
  viewer.start(8080);
  printf("Web interface running at http://localhost:8080\nPress Enter to"
         "exit...\n");

  // Create elements (filepath, id, location)
  Element elem1("img1.jpg", 1, cv::Point(50, 50),
                cv::Size(128, 128)); // 64x64 pixels

  printf("Adding elements to canvas...\n");
  // Add elements to the canvas
  vCanvas.addElementToCanvas(elem1);

  viewer.updateCanvas(vCanvas.getPixelMatrix());

  // Wait for a few seconds
  std::this_thread::sleep_for(std::chrono::seconds(15));

  printf("Removing element 1 to canvas...\n");
  // Remove one element
  vCanvas.removeElementFromCanvas(elem1);

  // Update web viewer with new state
  viewer.updateCanvas(vCanvas.getPixelMatrix());

  // Keep program running to serve web interface
  std::cin.get();

  return 0;
}
