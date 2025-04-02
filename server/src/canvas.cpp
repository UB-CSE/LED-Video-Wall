#include "canvas.h"            
#include "input-parser.hpp"    
#include <algorithm>
#include <iostream>


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




/*
Push changes to canvas-

This clears the virtual canvas, sorts the elementlist, then displays the head element of every vector
in the element list.

*/
void VirtualCanvas::pushToCanvas(){


    // Sort by ID of the first element in each tuple's vector
    std::sort(elementList.begin(), elementList.end(), [](const ElemTuple& a, const ElemTuple& b) {
        const std::vector<Element>& vecA = std::get<2>(a);
        const std::vector<Element>& vecB = std::get<2>(b);
        return vecA.front().getId() < vecB.front().getId();
    });

    clear();

    for (const ElemTuple& tuple : elementList) {
        const std::vector<Element>& vec = std::get<2>(tuple);


        const Element& elem = vec.at(0);
        cv::Point loc = elem.getLocation();
        cv::Mat elemMat = elem.getPixelMatrix();
        cv::Size elemSize = elem.getDimensions();

        //Crop to fit if it surpasses canvas dimensions
        if (loc.x + elemSize.width > dim.width) {
            elemSize.width = dim.width - loc.x;
        }
        if (loc.y + elemSize.height > dim.height) {
            elemSize.height = dim.height - loc.y;
        }

        elemMat = elemMat(cv::Rect(0, 0, elemSize.width, elemSize.height));
        elemMat.copyTo(pixelMatrix(cv::Rect(loc, elemSize)));
    }
}

/*
Update

This method looks at all the element vectors in the elementlist held by the virtual canvas. For each
that has more than one member, it moves the head to the tail. Then the method pushes changes to the
canvas. 

The purpose of this is to allow for carousel and potentially videos. pushToCanvas pushes the front
elements of every element vector in element list to the canvas. As such, if we cycle the
individual vectors, we get the next frame.
*/

void VirtualCanvas::updateCanvas(){

    for(ElemTuple & tup : elementList){

        int callsPerUpdate = std::get<0>(tup);
        if(callsPerUpdate == -1){break;}
        int curUpdate = std::get<1>(tup);

        if(callsPerUpdate == curUpdate){
            auto it = std::get<2>(tup).begin();
            std::rotate(it, it + 1,std::get<2>(tup).end());
        }

        std::get<1>(tup) = (curUpdate + 1) % callsPerUpdate;
       
    }

    pushToCanvas();

}


/*
Adds an element to the canvas at its defined location set in the element itself

Note that "element" in this context is a generic vector of tupled elements. These
tuples are (int updates, Element element) where updates is the number of update calls 
before updating.

Static images have vectors of size one, each containing one element.
Carousels have vectors containing the number of constituent elements.

*/
void VirtualCanvas::addElementToCanvas(const ElemTuple& element) {

    std::vector<ElemTuple> objects = getElementList();
    int elementID = std::get<2>(element).front().getId();

    /*
    This searches the elementList to check if the element being added already exists. If that is true, Throw error and return.

    NOTE: This small code block was chatgpted.

    Its intended purpose is to traverse a vector of vector of elements and throw a fit if the element we are trying to add already has its
    id present in the vector..
    
    */
   auto it = std::find_if(objects.begin(), objects.end(), [elementID](const ElemTuple& tup) {
        const std::vector<Element>& vec = std::get<2>(tup);
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

        for (ElemTuple tup : elementsPayload["images"]) {
            VirtualCanvas::addElementToCanvas(tup);
        }

    }
    
    //IF THERE ARE CAROUSELS
    if (!elementsPayload["carousel"].empty()){

        for (ElemTuple tup : elementsPayload["carousel"]) {
            VirtualCanvas::addElementToCanvas(tup);
        }


    }
    
    
}
void VirtualCanvas::removeElementFromCanvas(int elementId) {
    clear();

    for (size_t i = 0; i < elementList.size(); i++) {
        ElemTuple& tuple = elementList[i];
        std::vector<Element> vec = std::get<2>(tuple);

        if (!vec.empty() && vec.front().getId() == elementId) {
            elementList.erase(elementList.begin() + i);
            elementCount--;
            break;
        }
    }

    pushToCanvas();
}

