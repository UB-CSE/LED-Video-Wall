#include "canvas.h"

// Constructor with size control
Element::Element(const std::string &path, int elementId, cv::Point loc,
                 const cv::Size &targetSize)
    : filePath(path), location(loc), id(elementId) {

  // Read Image
  pixelMatrix = cv::imread(filePath, cv::IMREAD_COLOR);

  if (pixelMatrix.empty()) {
    std::cerr << "Error: Could not load image at " << filePath << std::endl;
  } else {
    if (targetSize.width > 0 && targetSize.height > 0) {
      cv::resize(pixelMatrix, pixelMatrix, targetSize, 0, 0, cv::INTER_AREA);
    }
    dim = pixelMatrix.size();
  }
}

// Original constructor for backward compatibility
Element::Element(const std::string &path, int elementId, cv::Point loc)
    : Element(path, elementId, loc, cv::Size(0, 0)) {}

// Override of abstract class method
void Element::clear() { pixelMatrix = cv::Mat::zeros(dim, pixelMatrix.type()); }

// VirtualCanvas implementation
VirtualCanvas::VirtualCanvas(const cv::Size &size)
    : AbstractCanvas(size), elementCount(0) {}

void VirtualCanvas::clear() { pixelMatrix = cv::Mat::zeros(dim, CV_8UC3); }

void VirtualCanvas::addElementToCanvas(const Element &element) {
  cv::Point loc = element.getLocation();
  cv::Mat elemMat = element.getPixelMatrix();
  cv::Size elemSize = element.getDimensions();

  // Overwrite a region of interest with the image
  elemMat.copyTo(pixelMatrix(cv::Rect(loc, elemSize)));

  // Store the element in the list
  elementList.push_back(element);
  elementCount++;
}

void VirtualCanvas::removeElementFromCanvas(const Element &element) {
  clear();

  for (size_t i = 0; i < elementList.size(); i++) {
    if (elementList[i].getId() == element.getId()) {
      elementList.erase(elementList.begin() + i);
      elementCount--;
      break;
    }
  }

  // Re-add remaining elements
  for (const Element &elem : elementList) {
    addElementToCanvas(elem);
  }
}
