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

=====================================================================================
             Welcome to Harvey's Virtual Canvas and Element tutorial
=====================================================================================

Notes:

When merging the Text functionality back in, it should be drag and drop.







=====================================================================================
                                  INPUT HANDLING
=====================================================================================
By default, elements are read from the input.yaml. Currently supports static images
and carousels. Please see sample input file, "input.yaml" for more details.

Parsing the input file yields an elementPayload. This is a map with keys relating to 
the different element types and a corresponding element vector vector. Each vector 
within the primary value vector holds all the relevant elements for a specific entry. 


For example, elementPayload["images"] is a vector of vector of elements | where the
vector is populated by vectors of size one, containing each static image element 
defined in the config. 

elementPayload["images"] = {{1}, {2}, {4}, ...}    

(Numbers indicate an element with that id.)

However something like elementPayload["carousel"] is the same but the inner vectors 
are variable length; each containing all the elements that make up one carousel. All 
member elements of a distinct carousel have the same ID (never will more than one be 
on screen at a time).

elementPayload["carousel"] = {{1,1,1}, {2,2}, {4,4,4,4,4}, ...}


*/


int main() {

    std::string inputFilePath = "input.yaml";
    VirtualCanvas vCanvas(cv::Size(2000, 2000));
    
    /*
    =====================================================================================
                   CONFIG LOADING AND INITIAL VIRTUAL CANVAS DISPLAY
    =====================================================================================

    Payload(s) are "std::map<std::string, std::vector<ElemVec>>"

    ElemVec(s) are "std::vector<Element>"

    */

    Payload elementsPayload = parseInput(inputFilePath);
    
    vCanvas.addPayloadToCanvas(elementsPayload);
    cv::imshow("Display Cats", vCanvas.getPixelMatrix());
    cv::waitKey(0);


    /*
    =====================================================================================
                                    CANVAS UPDATING
    =====================================================================================

    Updates all multiframe elements by one time step (carousels/videos/etc). 
    This also loops all the elements so you can update infinitely.
    Carousels can be removed just like any other element with removeElementFromCanvas. 

    To show a carousel, make a loop that calls update and show/read every x seconds. 


    =====================================================================================
                                    FRAMERATE CONTROL
    =====================================================================================
    To control framerate, there are two things required. The master framerate and the 
    local "real" framerate.

    Users can specify the real framerate in the input configuration (see input.yaml). We 
    on the backend must specify the master framerate. See (input-parser.cpp), where it is 
    currently fixed with a global definition "MASTER_FRAMERATE"

    When we call update on the virtual canvas, elements are mathed out to update in sync 
    with the master framerate ---

    Concretely, we call update on every tick of the master frame rate, and the internal 
    virtual canvas will update once every X calls, to match the real frame rate defined 
    by the user.

    This requires the master loop to read in sync with the updates as well. (handled in
    main server master loop)

    */

    
    /*
    =====================================================================================
                                    HARVEY DEMO
    =====================================================================================
    Here I have prepared a carousel with 3 images. The first image of the carousel is 
    displayed during config load along with the other static images already.

    In the input file, I have designated the carousel to run at 30 fps. I currently have 
    the master update tick rate fixed in code at 60 per second. Hence, the carousel will 
    update once every two update calls. 

    */

    //No Change to carousel
    vCanvas.updateCanvas();
    cv::imshow("Display Cats", vCanvas.getPixelMatrix());
    cv::waitKey(0);

    //Change!
    vCanvas.updateCanvas();
    cv::imshow("Display Cats", vCanvas.getPixelMatrix());
    cv::waitKey(0);

    //No Change to carousel
    vCanvas.updateCanvas();
    cv::imshow("Display Cats", vCanvas.getPixelMatrix());
    cv::waitKey(0);
    
    //Change!
    vCanvas.updateCanvas();
    cv::imshow("Display Cats", vCanvas.getPixelMatrix());
    cv::waitKey(0);

    //No Change to carousel
    vCanvas.updateCanvas();
    cv::imshow("Display Cats", vCanvas.getPixelMatrix());
    cv::waitKey(0);
    
    //Change! - Loops back to first image and so on!
    vCanvas.updateCanvas();
    cv::imshow("Display Cats", vCanvas.getPixelMatrix());
    cv::waitKey(0);
    


    //This is how you removes a specific element. Pass it the element id.
    vCanvas.removeElementFromCanvas(1);
    cv::imshow("Display Cats", vCanvas.getPixelMatrix());
    cv::waitKey(0);


    return 0;
}


