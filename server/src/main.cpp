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


*/


int main() {

    std::string inputFilePath = "input.yaml";
    VirtualCanvas vCanvas(cv::Size(2000, 2000));
    
    
    parseInput(vCanvas, inputFilePath);
    
    cv::imshow("Display Cats", vCanvas.getPixelMatrix());
    cv::waitKey(0);

    while (true) {
        for (int i = 0; i < vCanvas.getElementList().size(); i++) {
            cv::Mat frame;
            vCanvas.getElementList().at(i)->nextFrame(frame);
        }
    
        vCanvas.pushToCanvas();
        cv::imshow("Display Cats", vCanvas.getPixelMatrix());
    
        if (cv::waitKey(33) >= 0) { //in milliseconds
            break;
        }
    }

    




    return 0;
}


