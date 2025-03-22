#include <cstdio>
#include <exception>
#include <iostream>
#include <iterator>
#include <string.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h> // for close
#include "tcp.hpp"
#include "client.hpp"
#include "config-parser.hpp"
#include <vector>
#include "canvas.h"
#include "input-parser.hpp"
#include <opencv2/opencv.hpp>






/*
Welcome to Harvey's Virtual Canvas and Element tutorial

By default, elements are read from the input.yaml. Currently supports static images
and carousels. Please see sample input file for more details.

Parsing the input file yields an elementPayload. This is a map with keys relating to the different
element types and a corresponding element vector vector. Each vector within the primary value vector
holds all the relevant elements for a specific entry. 


For example, elementPayload["images"] is a vector of vector of elements | where the vector is populated
by vectors of size one, containing each static image element defined in the config. 

elementPayload["images"] = {{1}, {2}, {4}, ...}    (Numbers indicate an element with that id.)

However something like elementPayload["carousel"] is the same but the inner vectors are variable length;
each containing all the elements that make up one carousel. All member elements of a distinct carousel
have the same ID (never will more than one be on screen at a time).

elementPayload["carousel"] = {{1,1,1}, {2,2}, {4,4,4,4,4}, ...}


*/


int main() {

    std::string inputFilePath = "input.yaml";
    VirtualCanvas vCanvas(cv::Size(2000, 2000));
    
    /*
    Load Config
    */

    std::map <std::string, std::vector<std::vector<Element>>> elementsPayload = parseInput(inputFilePath);
    
    vCanvas.addPayloadToCanvas(elementsPayload);
    cv::imshow("Display Cats", vCanvas.getPixelMatrix());
    cv::waitKey(0);


    /*
    updateCanvas - 

    Updates all multiframe elements by one time step (carousels/videos/etc). 
    This also loops all the elements so you can update infinitely.
    Carousels can be removed just like any other element with removeElementFromCanvas. 

    To show a carousel, make a loop that calls update and show every x seconds. 
    */


    //Here I have prepared a carousel with 3 images. The first is displayed during config load already
    vCanvas.updateCanvas();
    cv::imshow("Display Cats", vCanvas.getPixelMatrix());
    cv::waitKey(0);
    vCanvas.updateCanvas();
    cv::imshow("Display Cats", vCanvas.getPixelMatrix());
    cv::waitKey(0);


    //This is how you removes a specific element. Pass it the element id.
    vCanvas.removeElementFromCanvas(1);
    cv::imshow("Display Cats", vCanvas.getPixelMatrix());
    cv::waitKey(0);


    return 0;
}


