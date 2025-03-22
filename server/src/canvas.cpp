#include "canvas.h"
#include <algorithm>

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

    std::vector<Element> objects = getElementList();
    int elementID = element.getId();


    //This searches the elementList to check if the element being added already exists. If that is true, Throw error and return.
    auto it = std::find_if(objects.begin(), objects.end(),
                           [elementID](const Element& obj) {
                               return obj.getId() == elementID;
                           });

    if (it != objects.end()){
        std::cout << "Double loading element ID# " << elementID << std::endl;
        return;
    }



    cv::Point loc = element.getLocation();
    cv::Mat elemMat = element.getPixelMatrix();
    cv::Size elemSize = element.getDimensions();


    /*
    Overwite a region of interest with the image. If the image does not fit on the canvas,
    we derive a new size and crop the element to it before transferring it to the canvas.
    */

    if(loc.x + elemSize.width > dim.width){
        elemSize.width = dim.width-loc.x;
    }else if (loc.y + elemSize.height > dim.height){
        elemSize.height = dim.height - loc.y;
    }
    
    elemMat = elemMat(cv::Rect(0, 0, elemSize.width, elemSize.height));
    elemMat.copyTo(pixelMatrix(cv::Rect(loc, elemSize)));

    //Store the element in the list
    elementList.push_back(element);
    elementCount++;
}


//Adds an entire map of elements to the canvas. It sorts elements by ID first s.t higher IDs are like weights, their images go on top
void VirtualCanvas::addPayloadToCanvas(std::map <std::string, std::vector<std::vector<Element>>>& elementsPayload){
    clear();

    //IF THERE ARE IMAGES
    if(elementsPayload["images"].size() != 0){

        /*
        Flattens the element vector vector key of "images" into a one dimensional element vector. Then proceeds as before
        https://stackoverflow.com/questions/17294629/merging-flattening-sub-vectors-into-a-single-vector-c-converting-2d-to-1d
        */

        std::vector<Element> elementsVec;
        // Optionally, preallocate if you know the total size.
        for (const auto& vec : elementsPayload["images"]) {
            elementsVec.insert(elementsVec.end(), vec.begin(), vec.end());
        }

        std::sort(elementsVec.begin(), elementsVec.end(), [](const Element &a, const Element &b) {
            return a.getId() < b.getId();
        });
    
        for(const auto& element : elementsVec){
            VirtualCanvas::addElementToCanvas(element);
        }


    }//IF THERE ARE CAROUSELS
    else if (elementsPayload["carousel"].size() != 0){

    }
    else{


    }
    
    
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
