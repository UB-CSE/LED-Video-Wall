#include "canvas.h"
#include "config-parser.hpp"
#include "controller_region.h"
#include "web_viewer.h"
#include <chrono>
#include <cstdio>
#include <iostream>
#include <map>
#include <opencv2/opencv.hpp>
#include <random>
#include <thread>

int main(int argc, char *argv[]) {
  const std::string imagePath = "./images/12x6.jpg";

  if (argc <= 1) {
    std::cout << "Must specify matrix configuration path. \n"
              << "usage: canvas_test <config path>\n";
    return -1;
  }

  cv::Point maxPoint(512, 512);

  VirtualCanvas vCanvas(cv::Size(maxPoint.x, maxPoint.y));
  printf("Created canvas with size %dx%d\n", maxPoint.x, maxPoint.y);

  // Create controller mapper from configuration file
  auto mapper =
      std::make_shared<ControllerMapper>(vCanvas.getDimensions(), argv[1]);

  printf("Controller mapper initialized with %zu regions\n",
         mapper->getRegions().size());

  printf("Region details:\n");
  for (const auto &region : mapper->getRegions()) {
    printf("Region ID: %d, Pixels: %zu, Bounding Box: (%d,%d,%d,%d)\n",
           region.getId(), region.getPixels().size(), region.getBoundingBox().x,
           region.getBoundingBox().y, region.getBoundingBox().width,
           region.getBoundingBox().height);
  }

  try {
    Element imageElement(imagePath, 1, cv::Point(0, 0));

    // Add the element to our virtual canvas
    vCanvas.addElementToCanvas(imageElement);

    printf("Added image '%s' to canvas at position (100,100)\n",
           imagePath.c_str());

    // Get regions affected by this new element, we'll use this to know which
    // microcontrollers need to be updated when sending out new data
    auto affectedRegions =
        mapper->getAffectedRegions(imageElement, vCanvas.getPixelMatrix());
    printf("Image affects %zu controller regions\n", affectedRegions.size());

  } catch (const std::exception &e) {
    printf("Error loading image: %s\n", e.what());
  }

  WebViewer viewer;
  viewer.start(8080);
  printf("Web interface running at http://localhost:8080\n");

  while (true) {
    viewer.updateCanvas(vCanvas.getPixelMatrix());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return 0;
}
