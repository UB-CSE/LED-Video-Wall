#include "canvas.h"
#include <algorithm>

//Constructor: Loads an image from file
Element::Element(const std::string& path, int elementId, cv::Point loc): filePath(path), location(loc), id(elementId){

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

//Push changes to canvas
void VirtualCanvas::pushToCanvas(){


    // REMEMBER TO EXPLAIN WHY WE ONLY SORT THE FRONT - EXPLAIN GUARENTEE OF SAME ID
    std::sort(elementList.begin(), elementList.end(), [](const std::vector<Element> &a, const std::vector<Element> &b) {
        return a.front().getId() < b.front().getId();
    });


    clear();
    for (const std::vector<Element>& wrappedElem : elementList) {

        const Element elem = wrappedElem.at(0);

        cv::Point loc = elem.getLocation();
        cv::Mat elemMat = elem.getPixelMatrix();
        cv::Size elemSize = elem.getDimensions();


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
        
    }
}



//Edit this to take vector of elements


//Adds an element to the canvas at its defined location set in the element itself
void VirtualCanvas::addElementToCanvas(const Element& element) {

    std::vector<std::vector<Element>> objects = getElementList();
    int elementID = element.getId();

    /*
    This searches the elementList to check if the element being added already exists. If that is true, Throw error and return.

    NOTE: This small code block was chatgpted.

    Its intended purpose is to traverse a vector of vector of elements and throw a fit if the element we are trying to add already has its
    id present in the vector..
    
    */
    auto it = std::find_if(objects.begin(), objects.end(), [elementID](const std::vector<Element>& vec) {
        return std::any_of(vec.begin(), vec.end(), [elementID](const Element& obj) {
            return obj.getId() == elementID;
        });
    });
    if (it != objects.end()){
        std::cout << "Double loading element ID# " << elementID << std::endl;
        return;
    }

    //Store the element in the list
    std::vector<Element> wrappedElement = {element};
    elementList.push_back(wrappedElement);
    elementCount++;

    pushToCanvas();
   
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

void VirtualCanvas::removeElementFromCanvas(int elementId) {
    clear();

    for (size_t i = 0; i < elementList.size(); i++) {
        if (elementList[i].front().getId() == elementId) {
            elementList.erase(elementList.begin() + i);
            elementCount--;
            break;
        }
    }

    pushToCanvas();
}
