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

void canvas_debug(VirtualCanvas& vCanvas, std::string inputFilePath);

int main() {

    std::string inputFilePath = "input.yaml";
    VirtualCanvas vCanvas(cv::Size(2000, 2000));
    canvas_debug(vCanvas, inputFilePath);
    return 0;
}

void canvas_debug(VirtualCanvas& vCanvas, std::string inputFilePath) {

    std::map <std::string, std::vector<std::vector<Element>>> elementsPayload = parseInput(inputFilePath);
    vCanvas.addPayloadToCanvas(elementsPayload);

    cv::imshow("Display Cats", vCanvas.getPixelMatrix());
    cv::waitKey(0);



    
}
