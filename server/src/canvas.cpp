// clang-format off
#include "input-parser.hpp"
#include "canvas.hpp"

#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <string>


//ImageElement implementation
ImageElement::ImageElement(const std::string& filepath, int id, cv::Point loc, int frameRate): Element(id, loc, frameRate) {
    pixelMatrix = cv::imread(filepath, cv::IMREAD_COLOR);
    if (pixelMatrix.empty()) {
        throw std::runtime_error("Failed to load image: " + filepath);
    }
}

bool ImageElement::nextFrame(cv::Mat& frame) {
    frame = pixelMatrix;
    return true;
}

void ImageElement::reset() {
    provided = false;
}


/*
Carousel Implementation

Maintains a vector of images it is responsible for.
Utilizes internal counter with modulo shenanigans to track which frame is in play

*/
CarouselElement::CarouselElement(const std::vector<std::string>& filepaths, int id, cv::Point loc, int frameRate): Element(id, loc,frameRate), current(0) {
    for (const auto& path : filepaths) {
        cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
        if (img.empty())
            throw std::runtime_error("Failed to load image: " + path);
        pixelMatrices.push_back(img); //This is an internal vector of images held by the carousel object
    }
    if (pixelMatrices.empty())
        throw std::runtime_error("No images loaded.");

    pixelMatrix = pixelMatrices[0].clone();  //Init current matrix with top matrix
}

bool CarouselElement::nextFrame(cv::Mat& frame) {
    frame = pixelMatrices[current];  // Update stored frame
    pixelMatrix = frame;
    current = (current + 1) % pixelMatrices.size();
    return true;
}

void CarouselElement::reset() {
    current = 0;
    pixelMatrix = pixelMatrices[0];
}

//VideoElement implementation

VideoElement::VideoElement(const std::string& filepath, int id, cv::Point loc, int frameRate): Element(id, loc, frameRate) {
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
}

//VideoElement implementation

VideoElement::VideoElement(int webcamNum, int id, cv::Point loc, int frameRate): Element(id, loc, frameRate) {
    // This does not work on wsl because we dont have native webcam access.
    cap.open(webcamNum);
    if (!cap.isOpened())
        throw std::runtime_error("Failed to open webcam: " + std::to_string(webcamNum));

    //Load first frame
    cap.read(pixelMatrix);
    cap.set(cv::CAP_PROP_POS_FRAMES, 0);
}


bool VideoElement::nextFrame(cv::Mat& frame) {
    if (!cap.read(pixelMatrix)) {
        //Rewind and try again
        cap.set(cv::CAP_PROP_POS_FRAMES, 0);
        if (!cap.read(pixelMatrix)) {
            frame = cv::Mat::zeros(pixelMatrix.size(), CV_8UC3); // fallback to empty to prevent crashes when ending
            return false;
        }
    }

    frame = pixelMatrix;
    return true;

}

void VideoElement::reset() {
    cap.set(cv::CAP_PROP_POS_FRAMES, 0);

    //Reload first frame back into pixelMatrix
    cap.read(pixelMatrix);
    cap.set(cv::CAP_PROP_POS_FRAMES, 0);
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

        std::cout << "Element ID: " << elemPtr->getId()
             << " at (" << loc.x << "," << loc.y << ")"
               << " size " << elemSize.width << "x" << elemSize.height << std::endl;


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


            elemMat.copyTo(pixelMatrix(cv::Rect(loc, elemSize)));
        }else{

            printf("\n Element with ID: %d was placed out of bounds and has not been loaded", elemPtr->getId());
        }  
        
    }
}

bool VirtualCanvas::moveElement(int elementId, cv::Point loc){

    std::vector<Element *> elementPtrs = getElementList();
    auto it = std::find_if(elementPtrs.begin(), elementPtrs.end(), [elementId](const Element* ptr) {
        return ptr && ptr->getId() == elementId;
    });

    if (it != elementPtrs.end()) {
        (*it)->getLocation() = loc;
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




