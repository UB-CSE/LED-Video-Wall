

#include "input-parser.hpp"
#include "canvas.h" 
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <string>
#include <vector>
#include <tuple>

#define MASTER_FRAMERATE 60



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

Payload parseInput(const std::string& inputFile) {
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
                    Element elem(path, id, loc, std::make_tuple(calcFramerate(MASTER_FRAMERATE, framerate), 0));
                    carouselArray.push_back(elem);
                }

                

                elementPayload["carousel"].push_back(carouselArray);
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