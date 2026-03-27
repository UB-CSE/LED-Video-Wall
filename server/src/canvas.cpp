// clang-format off
#include "input-parser.hpp"
#include "canvas.hpp"

#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <string>
#include <cmath>
#include <algorithm>

void Element::rotateFrame() {
    if (angleDegrees == 0.f || pixelMatrix.empty()) {
        return;
    }

    cv::Size frameSize = pixelMatrix.size();

    // Calculate how much bigger the element needs to be to fit the rotated frame without cropping

    double frameHalfWidth = frameSize.width / 2.0,
           frameHalfHeight = frameSize.height / 2.0;
    cv::Point2d frameCenter(frameHalfWidth, frameHalfHeight);
    double frameCenterToCornerAngleRad = std::atan(frameHalfHeight / frameHalfWidth);
    double frameCenterToCornerDist = std::hypot(frameHalfWidth, frameHalfHeight);

    double angleRad = angleDegrees * (M_PI / 180.0);

    auto extendAtAngle = [](cv::Point2d center, double dist, double theta) {
        double dx = dist * std::cos(theta);
        double dy = dist * std::sin(theta);
        return cv::Point2d(center.x + dx, center.y + dy);
    };

    cv::Point2d frameTopRightAfterRotation = extendAtAngle(frameCenter, frameCenterToCornerDist, angleRad + frameCenterToCornerAngleRad);
    cv::Point2d frameTopLeftAfterRotation = extendAtAngle(frameCenter, frameCenterToCornerDist, angleRad + M_PI - frameCenterToCornerAngleRad);

    double maxX = std::max(std::abs(frameCenter.x - frameTopRightAfterRotation.x), std::abs(frameCenter.x - frameTopLeftAfterRotation.x));
    double maxY = std::max(std::abs(frameCenter.y - frameTopRightAfterRotation.y), std::abs(frameCenter.y - frameTopLeftAfterRotation.y));

    int paddingX = static_cast<int>(std::ceil(std::max(maxX - frameHalfWidth, 0.0)));
    int paddingY = static_cast<int>(std::ceil(std::max(maxY - frameHalfHeight, 0.0)));

    // Expand the frame with a transparent border

    cv::Mat paddedFrame = cv::Mat::zeros(frameSize.height + 2 * paddingY, frameSize.width + 2 * paddingX, CV_8UC4);
    if (pixelMatrix.channels() == 3) {
        cv::cvtColor(pixelMatrix, pixelMatrix, cv::COLOR_BGR2BGRA);
    }
    pixelMatrix.copyTo(paddedFrame(cv::Rect(paddingX, paddingY, frameSize.width, frameSize.height)));

    cv::Size paddedFrameSize = paddedFrame.size();
    cv::Point2d paddedFrameCenter(frameHalfWidth + paddingX, frameHalfHeight + paddingY);

    // Rotate around the center

    cv::Mat rotationMatrix = cv::getRotationMatrix2D(paddedFrameCenter, angleDegrees, 1.0);
    cv::warpAffine(paddedFrame, pixelMatrix, rotationMatrix,paddedFrameSize);

    // Keep the center of the element in the same place on the canvas

    locationOffset = cv::Point(-paddingX, -paddingY);
}

//ImageElement implementation

ImageElement::ImageElement(const std::string& filepath, int id, cv::Point loc, double scale, double rotationDegreees) : Element(id, loc, -1, rotationDegreees) {
    pixelMatrix = cv::imread(filepath, cv::IMREAD_UNCHANGED);
    if (pixelMatrix.empty()) {
        throw std::runtime_error("Failed to load image: " + filepath);
    }
    original_ = pixelMatrix.clone();  
    filePath_ = filepath; 
    setScale(scale);
    rotateFrame();
}

void ImageElement::reset() {
    provided = false;
}


/*
Carousel Implementation

Maintains a vector of images it is responsible for.
Utilizes internal counter with modulo shenanigans to track which frame is in play

*/
CarouselElement::CarouselElement(const std::vector<std::string>& filepaths, int id, cv::Point loc, int frameRate, double rotationDegrees)  : Element(id, loc,frameRate, rotationDegrees), current(0) {
    for (const auto& path : filepaths) {
        cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
        if (img.empty())
            throw std::runtime_error("Failed to load image: " + path);
        pixelMatrices.push_back(img); //This is an internal vector of images held by the carousel object
    }
    if (pixelMatrices.empty())
        throw std::runtime_error("No images loaded.");

    pixelMatrix = pixelMatrices[0].clone();  //Init current matrix with top matrix
    rotateFrame();
}

bool CarouselElement::nextFrame() {
    pixelMatrix = pixelMatrices[current];  // Update stored frame
    current = (current + 1) % pixelMatrices.size();
    rotateFrame();
    return true;
}

void CarouselElement::reset() {
    current = 0;
    pixelMatrix = pixelMatrices[0];
}

//VideoElement implementation

VideoElement::VideoElement(const std::string& filepath, int id, cv::Point loc, int frameRate, double rotationDegrees): Element(id, loc, frameRate, rotationDegrees) {
    if(filepath.find("rtsp://") != std::string::npos){
        cap.open(filepath, cv::CAP_FFMPEG);
    }else{
        cap.open(filepath);
    }
    if (!cap.isOpened())
        throw std::runtime_error("Failed to open video: " + filepath);

    //Load first frame
    cap.read(pixelMatrix);
    cap.set(cv::CAP_PROP_POS_FRAMES, 0);
    rotateFrame();
}

//VideoElement implementation

VideoElement::VideoElement(int webcamNum, int id, cv::Point loc, int frameRate, double rotationDegrees) : Element(id, loc, frameRate, rotationDegrees) {
    // This does not work on wsl because we dont have native webcam access.
    cap.open(webcamNum);
    if (!cap.isOpened())
        throw std::runtime_error("Failed to open webcam: " + std::to_string(webcamNum));

    //Load first frame
    cap.read(pixelMatrix);
    cap.set(cv::CAP_PROP_POS_FRAMES, 0);
    rotateFrame();
}


bool VideoElement::nextFrame() {
    if (!cap.read(pixelMatrix)) {
        //Rewind and try again
        cap.set(cv::CAP_PROP_POS_FRAMES, 0);
        if (!cap.read(pixelMatrix)) {
            return false;
        }
    }

    rotateFrame();

    return true;
}

void VideoElement::reset() {
    cap.set(cv::CAP_PROP_POS_FRAMES, 0);

    //Reload first frame back into pixelMatrix
    cap.read(pixelMatrix);
    cap.set(cv::CAP_PROP_POS_FRAMES, 0);
}

// RTMPStreamElement implementation

RTMPStreamElement::RTMPStreamElement(RTMPServer& rtmpServer, const std::string& streamName, int id, cv::Point loc, int frameRate, cv::Size size, double rotationDegrees) : Element(id, loc, frameRate, rotationDegrees), rtmpServer(rtmpServer), streamName(streamName), size(size) {
    reset();
}

bool RTMPStreamElement::nextFrame() {
  std::optional<cv::Mat> receivedFrame = rtmpServer.receiveStreamFrame(streamName);
  if (receivedFrame.has_value()) {
    pixelMatrix = receivedFrame.value();

    cv::Size frameSize = pixelMatrix.size();

    if (size != cv::Size(0, 0) && frameSize != cv::Size(0, 0)) {
      // preserve aspect ratio, do no exceed specified size
      double aspectRatio = static_cast<double>(frameSize.width) / frameSize.height;
      int newWidth = size.width;
      int newHeight = static_cast<int>(newWidth / aspectRatio);
      if (newHeight > size.height) {
        newHeight = size.height;
        newWidth = static_cast<int>(newHeight * aspectRatio);
      }
      cv::Size newSize(newWidth, newHeight);
      
      cv::resize(pixelMatrix, pixelMatrix, newSize);
    }

    rotateFrame();
  }

  return true;
}

void RTMPStreamElement::reset() {
    pixelMatrix = noFrameMat.clone();
    
    if (size != cv::Size(0, 0)) {
      cv::resize(pixelMatrix, pixelMatrix, size);
    }
    rotateFrame();
}

// TextElement implementation

TextElement::TextElement(const cv::Mat& imgBGR, int id, cv::Point loc, const std::string& text, const std::string& font, int size, cv::Scalar col, double rotationDegrees)
    : Element(id, loc, -1, rotationDegrees), content(text), fontPath(font), fontSize(size), color(col) {
    pixelMatrix = imgBGR.clone();
    rotateFrame();
}

/*
Adds an element pointer to the virtual canvas list element pointer list

This also checks to see if an element with the same ID has been loaded already
*/

void VirtualCanvas::addElementToCanvas(Element* element) {

    std::vector<Element *> elementPtrs = getElementList();
    int elementID = element->getId();

    /*
    This searches the elementPtrs vector to check if an element with the same ID already exists.
    If found, it throws an error and returns.
    */

    auto it = std::find_if(elementPtrs.begin(), elementPtrs.end(), [elementID](const Element* ptr) {
            return ptr && ptr->getId() == elementID;
        });

    if (it != elementPtrs.end()) {
        std::cout << "\nDouble loading element ID# " << elementID << std::endl;
        return;
    }

    //Store the element pointer in the list
    elementPtrList.push_back(element);
    elementCount++;

    pushToCanvas();

    


}


/*
Push changes to canvas-

This clears the virtual canvas, sorts the elementPtrList, then pushes the appropriate matrices to the virtual canvas.

This also serves the same function as the update command now. nextFrame() cycles to the next appropriate frame per call.

*/
void VirtualCanvas::pushToCanvas(){


    //Sort the element pointer list to respect layer weights
    std::sort(elementPtrList.begin(), elementPtrList.end(), [](const Element* a, const Element* b) {
        return a->getId() < b->getId();
    });

    //Clear to remove everything on the matrix
    clear();

    //Add all elements to the canvas in new order
    for (Element * elemPtr : elementPtrList) {

        cv::Point loc = elemPtr->getLocation();

        //Gets the current frame of the element object referenced by elemPtr
        cv::Mat elemMat = elemPtr->getPixelMatrix().clone();

        cv::Size elemSize = elemMat.size();

        if (elemSize.width <= 0 || elemSize.height <= 0) {
            continue;
        }

        /*std::cout << "Element ID: " << elemPtr->getId()
             << " at (" << loc.x << "," << loc.y << ")"
               << " size " << elemSize.width << "x" << elemSize.height << std::endl;*/


        /*
        Overwite a region of interest with the image. If the image does not fit on the canvas,
        we derive a new size and crop the element to it before transferring it to the canvas.
        */


        if((loc.x <= dim.width) && (loc.y <= dim.height)){

            if(loc.x + elemSize.width > dim.width){
                elemSize.width = dim.width-loc.x;
            }
            
            if (loc.y + elemSize.height > dim.height){
                elemSize.height = dim.height - loc.y;
            }

            
            
            elemMat = elemMat(cv::Rect(0, 0, elemSize.width, elemSize.height));

            //Apply the gamma LUT here - OpenCV DOES support in place lutting
            cv::LUT(elemMat, canvasLut, elemMat);

            overlayImage(elemMat, cv::Rect(loc, elemSize));
        }else{

            printf("\n Element with ID: %d was placed out of bounds and has not been loaded", elemPtr->getId());
        }  
        
    }
}

void VirtualCanvas::overlayImage(const cv::Mat& overlay, cv::Rect roi) {
    int offsetX = 0, offsetY = 0;
    if (roi.x < 0) {
        offsetX = -roi.x;
        roi.width -= offsetX;
        roi.x = 0;
    }
    if (roi.y < 0) {
        offsetY = -roi.y;
        roi.height -= offsetY;
        roi.y = 0;
    }

    roi.width = std::min(roi.width, dim.width - roi.x);
    roi.height = std::min(roi.height, dim.height - roi.y);

    if (overlay.channels() == 4) {
        // Overlay image manually going pixel by pixel.
        for (int y = roi.y; y < roi.y + roi.height; ++y) {
            uint8_t* canvasPtr = pixelMatrix.ptr<uint8_t>(y, roi.x);
            const uint8_t* overlayPtr = overlay.ptr<uint8_t>(y - roi.y + offsetY, offsetX);

            for (int x = 0; x < roi.width; ++x) {
                const uint8_t* in = overlayPtr + (x * 4);
                uint8_t* out = canvasPtr + (x * 3);

                const uint16_t alpha = in[3];
                if (alpha == 255) {
                    out[0] = in[0];
                    out[1] = in[1];
                    out[2] = in[2];
                } else {
                    // Blending with integer math, faster than using floating point
                    // (src * alpha + dst * (255 - alpha)) / 255
                    out[0] = static_cast<uint8_t>((in[0] * alpha + out[0] * (255 - alpha)) >> 8);
                    out[1] = static_cast<uint8_t>((in[1] * alpha + out[1] * (255 - alpha)) >> 8);
                    out[2] = static_cast<uint8_t>((in[2] * alpha + out[2] * (255 - alpha)) >> 8);
                }
            }
        }
    } else {
        overlay.copyTo(pixelMatrix(roi));
    }
}

bool VirtualCanvas::moveElement(int elementId, cv::Point loc){

    std::vector<Element *> elementPtrs = getElementList();
    auto it = std::find_if(elementPtrs.begin(), elementPtrs.end(), [elementId](const Element* ptr) {
        return ptr && ptr->getId() == elementId;
    });

    if (it != elementPtrs.end()) {
        (*it)->setLocation(loc);
    }

    return 0;

}


bool VirtualCanvas::removeElementFromCanvas(int elementId) {
    clear();

    for (size_t i = 0 ; i < elementPtrList.size(); i++) {
        Element* elem = elementPtrList.at(i);
        if (elem && elem->getId() == elementId) {
            delete elem;  //clean up memory
            elementPtrList.erase(elementPtrList.begin() + i);  //Needs to be an interator
            elementCount--;
            break;
        }
    }

    pushToCanvas();
    return 0;
}
