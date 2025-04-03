// clang-format off
#include "input-parser.hpp"
#include "canvas.h"
#include "text-render.hpp"
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <string>
#include <optional>
#include <vector>
#include <tuple>

// forward declarations
cv::Scalar hexColorToScalar(const std::string &hexColor);

#define MASTER_FRAMERATE 60


//Forward declaration
cv::Scalar hexColorToScalar(const std::string &hexColor);


//Calculates callsPerUpdate for elements
int calcFramerate(int masterFramerate, int targetFPS){

    return masterFramerate / targetFPS;
}

/*
New output format: Payload

images -> 

    vector of (ElemVec))

carousel -> same
videos -> same

*/

Payload parseInput(const std::string& inputFile, int64_t tick_rate) {
    Payload elementPayload;


    elementPayload["images"] = {};
    elementPayload["carousel"] = {};

    try {
        YAML::Node config = YAML::LoadFile(inputFile);
        YAML::Node elements = config["elements"];

        if (!elements) {
            std::cerr << "No elements found in config." << std::endl;
            return elementPayload;
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
                    abort();
                }

                std::string filepath = value["filepath"].as<std::string>();
                std::vector<int> locVec = value["location"].as<std::vector<int>>();

                if (locVec.size() != 2 || locVec[0] < 0 || locVec[1] < 0) {
                    std::cerr << "Location for element " << key << " malformed." << std::endl;
                    abort();
                }

                cv::Point loc(locVec[0], locVec[1]);

                /*
                ELEMENT CONSTRUCTION HERE

                The tuple is the framerate data field. All elements have it. The first number is the calculated
                callsPerUpdate and the second should always be init to 0. That is the internal counter.

                */
                Element elem(filepath, id, loc, std::make_tuple(-1, 0));
                ElemVec elemVec = std::vector{elem};

                elementPayload["images"].push_back(elemVec);
            }

            /*
            
            Carousel Types
            
            */

            else if (type == "carousel") {
                if (!value["filepaths"] || !value["location"] || !value["framerate"]) {
                    std::cerr << "Missing filepaths, framerate, or location for element: " << key << std::endl;
                    abort();
                }

                std::vector<std::string> filepaths = value["filepaths"].as<std::vector<std::string>>();
                std::vector<int> locVec = value["location"].as<std::vector<int>>();
                int framerate = value["framerate"].as<int>();

                if (locVec.size() != 2 || locVec[0] < 0 || locVec[1] < 0) {
                    std::cerr << "Location for element " << key << " malformed." << std::endl;
                    abort();
                }

                cv::Point loc(locVec[0], locVec[1]);
                std::vector<Element> carouselArray;

                /*
                ELEMENT CONSTRUCTION HERE
                */

                for (const auto& path : filepaths) {
                    Element elem(path, id, loc, std::make_tuple(calcFramerate(tick_rate, framerate), 0));
                    carouselArray.push_back(elem);
                }

                

                elementPayload["carousel"].push_back(carouselArray);
            }

            /*
            TEXT TYPE
            */
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
        abort();
    }

    return elementPayload;
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
