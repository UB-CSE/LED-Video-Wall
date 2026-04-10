// clang-format off
#include "input-parser.hpp"
#include "canvas.hpp"
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <string>
#include <optional>
#include <vector>
#include <tuple>
#include "text-render.hpp"

// forward declarations
cv::Scalar hexColorToScalar(const std::string &hexColor);


void parseInput(VirtualCanvas& vCanvas,  std::string& inputFile, RTMPServer& rtmpServer) {

    try {
        YAML::Node config = YAML::LoadFile(inputFile);
        YAML::Node elements = config["elements"];
        
        /*
        =======================================================================
                        LUT calculations for gamma application.
        =======================================================================

        Specifically, this precomputes the lookup table for the given gamma 
        value so that we don't need to do compute per pixel per image

   
        */

        cv::Mat lut(1, 256, CV_8UC1);
        double gamma = config["settings"]["gamma"].as<double>();

        if(gamma < 0){
            throw std::invalid_argument("Bad gamma value given");
        }

        uchar* lutPtr = lut.ptr();
        for (int i = 0; i < 256; ++i) {
            lutPtr[i] = cv::saturate_cast<uchar>(pow(i / 255.0, gamma) * 255.0);
        }
        vCanvas.canvasLut = lut;



        /*
        =======================================================================
                                Element processing
        =======================================================================
        */

        if (!elements) {
            std::cerr << "No elements found in config." << std::endl;
            throw std::invalid_argument("Empty Config!");
        }

        for (YAML::const_iterator it = elements.begin(); it != elements.end(); ++it) {
            std::string key = it->first.as<std::string>();
            YAML::Node value = it->second;

            std::string type = value["type"].as<std::string>();
            int id = value["id"].as<int>();

            /*

            Image Types
            
            */

            if (type == "image") {
                if (!value["filepath"] || !value["location"]) {
                    std::cerr << "Missing filepath or location for element: " << key << std::endl;
     
                }
                
                std::string filepath = value["filepath"].as<std::string>();
                std::vector<int> locVec = value["location"].as<std::vector<int>>();

                if (locVec.size() != 2 || locVec[0] < 0 || locVec[1] < 0) {
                    std::cerr << "Location for element " << key << " malformed." << std::endl;

                }

                cv::Point loc(locVec[0], locVec[1]);

                /*
                ELEMENT CONSTRUCTION HERE
                
                */
                
                double scale = 1.0;
                if (value["scale"]) {
                    scale = value["scale"].as<double>();
                    
                } 

                double rotationDegrees = 0.0;
                if (value["rotation"]) {
                    rotationDegrees = value["rotation"].as<double>();
                }

                Element * elem = new ImageElement(filepath, id, loc, scale, rotationDegrees);
                vCanvas.addElementToCanvas(elem);
                
            }

            /*
            
            Carousel Types
            
            */

            else if (type == "carousel") {
                if (!value["filepaths"] || !value["location"] || !value["framerate"]) {
                    std::cerr << "Missing filepaths, framerate, or location for element: " << key << std::endl;

                }

                int frameRate = value["framerate"].as<int>();
                std::vector<std::string> filepaths = value["filepaths"].as<std::vector<std::string>>();
                std::vector<int> locVec = value["location"].as<std::vector<int>>();
                cv::Point loc(locVec[0], locVec[1]);

                if (locVec.size() != 2 || locVec[0] < 0 || locVec[1] < 0) {
                    std::cerr << "Location for element " << key << " malformed." << std::endl;

                }

                double rotationDegrees = 0.0;
                if (value["rotation"]) {
                    rotationDegrees = value["rotation"].as<double>();
                }

                Element * elem = new CarouselElement(filepaths, id, loc, frameRate, rotationDegrees);
                vCanvas.addElementToCanvas(elem);
            }

            /*
            Video
            */
            else if (type == "video") {
                if (!value["filepath"] || !value["location"] || !value["framerate"]) {
                    std::cerr << "Missing filepath or location for element: " << key << std::endl;
    
                }

                int frameRate = value["framerate"].as<int>();
                std::string filepath = value["filepath"].as<std::string>();
                std::vector<int> locVec = value["location"].as<std::vector<int>>();

                if (locVec.size() != 2 || locVec[0] < 0 || locVec[1] < 0) {
                    std::cerr << "Location for element " << key << " malformed." << std::endl;
        
                }

                cv::Point loc(locVec[0], locVec[1]);

                double rotationDegrees = 0.0;
                if (value["rotation"]) {
                    rotationDegrees = value["rotation"].as<double>();
                }

                Element * elem = new VideoElement(filepath, id, loc, frameRate, rotationDegrees);

                vCanvas.addElementToCanvas(elem);

            }
            /*
            Webcam
            */
            else if (type == "webcam") {
                if (!value["camera-number"] || !value["location"] || !value["framerate"]) {
                    std::cerr << "Missing filepath or location for element: " << key << std::endl;
    
                }

                int frameRate = value["framerate"].as<int>();
                int webcamNum = value["camera-number"].as<int>();
                std::vector<int> locVec = value["location"].as<std::vector<int>>();

                if (locVec.size() != 2 || locVec[0] < 0 || locVec[1] < 0) {
                    std::cerr << "Location for element " << key << " malformed." << std::endl;
        
                }

                cv::Point loc(locVec[0], locVec[1]);

                double rotationDegrees = 0.0;
                if (value["rotation"]) {
                    rotationDegrees = value["rotation"].as<double>();
                }

                Element * elem = new VideoElement(webcamNum, id, loc, frameRate, rotationDegrees);

                vCanvas.addElementToCanvas(elem);

            }
            /*
            Text
            */
            else if (type == "text") {
                // check for necessary values
                if (!value["content"] || !value["font_path"] || !value["location"] || !value["color"] || !value["size"]) {
                    std::cerr << "Missing required values for element: " << key << std::endl;
                    continue;
                }

                std::string filepath = value["font_path"].as<std::string>();
                std::string content  = value["content"].as<std::string>();
                int fontSize         = value["size"].as<int>();

                // parse hex color to cv::Scalar (expects "#RRGGBB" or "RRGGBB")
                std::string hexColor = value["color"].as<std::string>();
                cv::Scalar fontColor = hexColorToScalar(hexColor);

                // convert location to cv::Point (same validation style as other branches)
                std::vector<int> locVec = value["location"].as<std::vector<int>>();
                if (locVec.size() != 2 || locVec[0] < 0 || locVec[1] < 0) {
                    std::cerr << "Location for element " << key << " malformed." << std::endl;
                    continue;
                }
                cv::Point posPoint(locVec[0], locVec[1]);

                double rotationDegrees = 0.0;
                if (value["rotation"]) {
                    rotationDegrees = value["rotation"].as<double>();
                }

                // create the element via the renderer (returns Element* or nullptr)
                Element* elem = renderTextToElement(content, filepath, fontSize, fontColor, id, posPoint, rotationDegrees);
                if (!elem) {
                    std::cerr << "Error parsing config: text failed to render (check TTF path/permissions): " 
                                << filepath << std::endl;
                    continue;
                }

                vCanvas.addElementToCanvas(elem);
            }

            else if (type == "rtmp") {
                if (!value["stream-name"] || !value["location"] || !value["framerate"]){
                    std:: cerr << "Missing stream-name, location, or framerate for RTMP element:" << key << std:: endl;
                    continue;
                }

                std::string streamName = value["stream-name"].as<std::string>();
                int frameRate = value["framerate"].as<int>();
                std::vector<int> locVec = value["location"].as<std::vector<int>>();

                if (locVec.size() != 2 || locVec[0] < 0 || locVec[1] < 0){
                    std::cerr << "Location for the element" << key << " malformed" << std::endl;
                    continue;
                }

                cv:: Point loc(locVec[0], locVec[1]);

                cv::Size size(0, 0); // do not resize

                if (value["size"]) {
                  std::vector<int> sizeVec = value["size"].as<std::vector<int>>();

                  if (sizeVec.size() != 2 || sizeVec[0] <= 0 || sizeVec[1] <= 0){
                      std::cerr << "Size for the element" << key << " malformed" << std::endl;
                      continue;
                  }

                   size = cv::Size(sizeVec[0], sizeVec[1]);
                }

                double rotationDegrees = 0.0;
                if (value["rotation"]) {
                    rotationDegrees = value["rotation"].as<double>();
                }

                Element* elem = new RTMPStreamElement(rtmpServer, streamName, id, loc, frameRate, size, rotationDegrees);

                vCanvas.addElementToCanvas(elem);
            }

            else if (type == "web-browser") {
                if (!value["url"] || !value["location"] || !value["framerate"] || !value["size"]){
                    std:: cerr << "Missing url, location, framerate, or size for browser element:" << key << std:: endl;
                    continue;
                }

                std::string url = value["url"].as<std::string>();
                int frameRate = value["framerate"].as<int>();
                std::vector<int> locVec = value["location"].as<std::vector<int>>();
                std::vector<int> sizeVec = value["size"].as<std::vector<int>>();

                if (locVec.size() != 2 || locVec[0] < 0 || locVec[1] < 0){
                    std::cerr << "Location for the element" << key << " malformed" << std::endl;
                    continue;
                }

                cv::Point loc(locVec[0], locVec[1]);

                if (sizeVec.size() != 2 || sizeVec[0] <= 0 || sizeVec[1] <= 0){
                    std::cerr << "Size for the element" << key << " malformed" << std::endl;
                    continue;
                }

                cv::Size size(sizeVec[0], sizeVec[1]);

                cv::Size viewSize(0, 0);
                if (value["view-size"]) {
                  std::vector<int> viewSizeVec = value["view-size"].as<std::vector<int>>();

                  if (viewSizeVec.size() != 2 || viewSizeVec[0] <= 0 || viewSizeVec[1] <= 0){
                      std::cerr << "View Size for the element" << key << " malformed" << std::endl;
                      continue;
                  }

                   viewSize = cv::Size(viewSizeVec[0], viewSizeVec[1]);
                }

                double rotationDegrees = 0.0;
                if (value["rotation"]) {
                    rotationDegrees = value["rotation"].as<double>();
                }

                Element* elem = new WebBrowserElement(url, id, loc, frameRate, size, viewSize, rotationDegrees);

                vCanvas.addElementToCanvas(elem);
            }

            else {
                std::cerr << "Unsupported element type: " << type << std::endl;
            }
        }

    } catch (const YAML::Exception& e) {
        std::cerr << "Error parsing config: " << e.what() << std::endl;

    }
}

cv::Scalar hexColorToScalar(const std::string &hexColor) {
  if (hexColor.length() != 7 || hexColor[0] != '#') {
    // invalid, we'll just return black and warn (thanks nick).
    std::cerr << "Error parsing config: Hex color \"" << hexColor << "\" invalid." << std::endl;
    return cv::Scalar(0, 0, 0);
  }

  int r, g, b;
  sscanf(hexColor.c_str(), "#%02x%02x%02x", &r, &g, &b);

  return cv::Scalar(b, g, r);
}

