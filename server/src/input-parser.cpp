

#include "input-parser.hpp"
#include "canvas.h"
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <string>
#include <vector>
#include <tuple>


#define MASTER_FRAMERATE 60




/*
New output format: Payload

images -> 

    vector of (ElemVec))

carousel -> same
videos -> same

*/

void parseInput(VirtualCanvas& vCanvas,  std::string& inputFile) {

    try {
        YAML::Node config = YAML::LoadFile(inputFile);
        YAML::Node elements = config["elements"];

        if (!elements) {
            std::cerr << "No elements found in config." << std::endl;
            throw std::runtime_error("Empty Config!");
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

                The tuple is the framerate data field. All elements have it. The first number is the calculated
                callsPerUpdate and the second should always be init to 0. That is the internal counter.

                */
                Element * elem = new ImageElement(filepath, id, loc); //Remember to "delete elem" afterwards

                vCanvas.addElementToCanvas(elem);
            }

            /*
            
            Carousel Types
            
            */

            else if (type == "carousel") {
                if (!value["filepaths"] || !value["location"]) {
                    std::cerr << "Missing filepaths, framerate, or location for element: " << key << std::endl;

                }

                std::vector<std::string> filepaths = value["filepaths"].as<std::vector<std::string>>();
                std::vector<int> locVec = value["location"].as<std::vector<int>>();
                cv::Point loc(locVec[0], locVec[1]);

                if (locVec.size() != 2 || locVec[0] < 0 || locVec[1] < 0) {
                    std::cerr << "Location for element " << key << " malformed." << std::endl;

                }

                

                Element * elem = new CarouselElement(filepaths, id, loc);
                vCanvas.addElementToCanvas(elem);
            }

            /*
            Video
            */
            else if (type == "video") {
                if (!value["filepath"] || !value["location"]) {
                    std::cerr << "Missing filepath or location for element: " << key << std::endl;
    
                }

                std::string filepath = value["filepath"].as<std::string>();
                std::vector<int> locVec = value["location"].as<std::vector<int>>();

                if (locVec.size() != 2 || locVec[0] < 0 || locVec[1] < 0) {
                    std::cerr << "Location for element " << key << " malformed." << std::endl;
        
                }

                cv::Point loc(locVec[0], locVec[1]);

                Element * elem = new VideoElement(filepath, id, loc);

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

