// clang-format off
#include "input-parser.hpp"
#include "canvas.h"
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <string>
#include <optional>
#include <vector>
#include <tuple>

// forward declarations
cv::Scalar hexColorToScalar(const std::string &hexColor);

#define MASTER_FRAMERATE 60




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
                
                */
                Element * elem = new ImageElement(filepath, id, loc, -1); //Remember to "delete elem" afterwards

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

                

                Element * elem = new CarouselElement(filepaths, id, loc, frameRate);
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

                Element * elem = new VideoElement(filepath, id, loc, frameRate);

                vCanvas.addElementToCanvas(elem);

            }
            // TEXT TYPE
            else if (type == "text") {
                // check for necessary values
                if (!value["content"] || !value["font_path"] || !value["location"] || !value["color"] || !value["size"]) 
                {
                  std::cerr << "Missing required values for element: " << key << std::endl;
                  abort();
                }
                std::string filepath = value["font_path"].as<std::string>();
                std::string content = value["content"].as<std::string>();
                
                int fontSize = value["size"].as<int>();
                
                // parse hex color to cv::Scalar
                std::string hexColor = value["color"].as<std::string>();
                cv::Scalar fontColor = hexColorToScalar(hexColor);
                
                // convert location from same format to cv::Point as used by renderTextToElement
                std::vector<int> locVec = value["location"].as<std::vector<int>>();
                if ((locVec.size() != 2) || (locVec.at(0) < 0) || (locVec.at(1) < 0)) {
                  std::cerr << "Location for element " << key << " malformed." << std::endl;
                  abort();
                }
                cv::Point posPoint(locVec.at(0), locVec.at(1));
                
                std::optional<Element> newElement = renderTextToElement(content, filepath, fontSize, fontColor, id, posPoint); 

                if (!newElement.has_value()) {
                  std::cerr << "Error parsing config: text failed to render, is the TTF file path correct?" << std::endl;
                  abort();
                }
                std::vector<Element> wrappedElem = {newElement.value()};
                elementPayload["images"].push_back(wrappedElem);
            }
            else {
                std::cerr << "Unsupported element type: " << type << std::endl;
            }
        }

    } catch (const YAML::Exception& e) {
        std::cerr << "Error parsing config: " << e.what() << std::endl;

    }
}

