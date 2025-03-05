#include "canvas.h"



//Constructor: Loads an image from file
Element::Element(const std::string& path, int elementId, cv::Point loc): filePath(path), location(loc), id(elementId){

    //Read Image
    pixelMatrix = cv::imread(filePath, cv::IMREAD_COLOR);

    if (pixelMatrix.empty()) {
        std::cerr << "Error: Could not load image at " << filePath << std::endl;
    } else {
        dim = pixelMatrix.size();
    }
}

//Override of abstract class method. Must be present - Clears the element's pixel matrix by setting to black
void Element::clear() {
    pixelMatrix = cv::Mat::zeros(dim, pixelMatrix.type());
}




//Constructor: Initializes the virtual canvas with just the size
VirtualCanvas::VirtualCanvas(const cv::Size& size) : AbstractCanvas(size), elementCount(0) {}

//Override of abstract class method. Must be present - Clears the element's pixel matrix by setting to black
void VirtualCanvas::clear() {
    pixelMatrix = cv::Mat::zeros(dim, CV_8UC3);
}

//Adds an element to the canvas at its defined location set in the element itself
void VirtualCanvas::addElementToCanvas(const Element& element) {
    cv::Point loc = element.getLocation();
    cv::Mat elemMat = element.getPixelMatrix();
    cv::Size elemSize = element.getDimensions();


    //Overwite a region of interest with the image - FIX IN FUTURE TO ACCOUNT FOR BOUNDARIES
    elemMat.copyTo(pixelMatrix(cv::Rect(loc, elemSize)));

    //Store the element in the list
    elementList.push_back(element);
    elementCount++;
}

void VirtualCanvas::removeElementFromCanvas(const Element& element) {
    clear();

    for (size_t i = 0; i < elementList.size(); i++) {
        if (elementList[i].getId() == element.getId()) {
            elementList.erase(elementList.begin() + i);
            elementCount--;
            break;
        }
    }

    //Re-add
    for (const Element& elem : elementList) {
        addElementToCanvas(elem);
    }
}
