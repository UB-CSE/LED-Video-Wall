#include "input-parser.hpp"   
#include "canvas.h"            
 
#include <algorithm>
#include <iostream>


//Constructor: Loads element from filepath
Element::Element(const std::string& path, int elementId, cv::Point loc, std::tuple<int, int> frameData): filePath(path), location(loc), id(elementId), frameRateData(frameData){

    pixelMatrix = cv::imread(filePath, cv::IMREAD_COLOR);

    if (pixelMatrix.empty()) {
        std::cerr << "Error: Could not load image at " << filePath << std::endl;
    } else {
        dim = pixelMatrix.size();
    }
}


//Second Constructor: Loads element from matrix
Element::Element(const cv::Mat matrix, int elementId, cv::Point loc, std::tuple<int, int> frameData): location(loc), id(elementId), frameRateData(frameData){
    if (matrix.empty()) {
        std::cerr << "Error: Invalid matrix passed into Element constructor. " << std::endl;
    } else {
        pixelMatrix = matrix; 
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




/*
Push changes to canvas-

This clears the virtual canvas, sorts the elementlist, then displays the head element of every vector
in the element list on the virtual canvas.

*/
void VirtualCanvas::pushToCanvas(){


    // REMEMBER TO EXPLAIN WHY WE ONLY SORT THE FRONT - EXPLAIN GUARENTEE OF SAME ID
    std::sort(elementList.begin(), elementList.end(), [](const std::vector<Element> &a, const std::vector<Element> &b) {
        return a.front().getId() < b.front().getId();
    });


    clear();
    for (const std::vector<Element>& wrappedElem : elementList) {

        const Element elem = wrappedElem.front();

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

/*
Update

This method looks at all the element vectors in the elementlist held by the virtual canvas. For each
that has more than one member, it moves the head to the tail. Then the method pushes the changes to the
virtual canvas. 

The purpose of this is to allow for carousel and potentially videos. pushToCanvas pushes the front
elements of every element vector in element list to the canvas. As such, if we cycle the
individual vectors, we get the next frame.

Framerate is controlled by incrementing a counter and comparing it to a threshold. Both are held within
the element itself. 
*/

void VirtualCanvas::updateCanvas(){

    for(ElemVec & vec : elementList){

        //This skips over checking single image element vectors (single images)
        if(vec.size() > 1){
            auto[callsPerUpdate, curUpdate] = vec.front().getFrameRateData();

            
            if(callsPerUpdate == -1){fprintf(stderr, "\nThere is a non-image element ID %d with callsPerUpdate of -1!", vec.front().getId()); continue;}

            
            //This updates the current counter
            std::get<1>(vec.front().frameRateData) += 1;

            //Threshold Check for when to update. The -1 is to account for 0 indexing.
            if(callsPerUpdate-1 == curUpdate){
    
                std::get<1>(vec.front().frameRateData) = 0;
                auto it = vec.begin();
                std::rotate(it, it + 1,vec.end());
            }
        }
       
    }

    pushToCanvas();

}


/*
Adds an element to the canvas at its defined location set in the element itself

Note that "element" in this context is a generic vector of elements.

Static images have vectors of size one, each containing one element.
Carousels have vectors containing the number of constituent elements.

*/
void VirtualCanvas::addElementToCanvas(const ElemVec& element) {

    std::vector<ElemVec> objects = getElementList();
    int elementID = element.front().getId();

    /*
    This searches the elementList to check if the element being added already exists. If that is true, Throw error and return.

    Its intended purpose is to traverse a vector of vector of elements and throw a fit if the element we are trying to add already has its
    id present in the vector..
    
    */
   
    auto it = std::find_if(objects.begin(), objects.end(), [elementID](const std::vector<Element>& vec) {
        return std::any_of(vec.begin(), vec.end(), [elementID](const Element& obj) {
            return obj.getId() == elementID;
        });
    });
    if (it != objects.end()){
        std::cout << "\nDouble loading element ID# \n" << elementID << std::endl;
        return;
    } 


    //Store the element tuple in the list
    elementList.push_back(element);
    elementCount++;

    pushToCanvas();
   
}


/*
Processes elementPayload map and moves relevant elements to the elementList vector held by the virtual canvas
*/
void VirtualCanvas::addPayloadToCanvas(Payload& elementsPayload){
    clear();

    
    //IF THERE ARE IMAGES
    if(!elementsPayload["images"].empty()){

        for (ElemVec vec : elementsPayload["images"]) {
            VirtualCanvas::addElementToCanvas(vec);
        }

    }
    
    //IF THERE ARE CAROUSELS
    if (!elementsPayload["carousel"].empty()){

        for (ElemVec vec : elementsPayload["carousel"]) {
            VirtualCanvas::addElementToCanvas(vec);
        }


    }
    
    
}
void VirtualCanvas::removeElementFromCanvas(int elementId) {
    clear();

    for (size_t i = 0; i < elementList.size(); i++) {
        ElemVec& vec = elementList[i];

        if (!vec.empty() && vec.front().getId() == elementId) {
            elementList.erase(elementList.begin() + i);
            elementCount--;
            break;
        }
    }

    pushToCanvas();
}

