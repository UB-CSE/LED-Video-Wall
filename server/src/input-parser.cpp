// clang-format off
#include "canvas.h"
#include "text-render.hpp"
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <string>
#include <optional>
#include <vector>

// forward declarations
cv::Scalar hexColorToScalar(const std::string &hexColor);

/*
New output format: Map

images -> element vector vector : std::vector<std::vector<Element>>
carousel -> element vector vector : std::vector<std::vector<Element>>
videos -> element vector vector : std::vector<std::vector<Element>>

*/


std::map <std::string, std::vector<std::vector<Element>>> parseInput(const std::string inputFile) {


    std::map <std::string, std::vector<std::vector<Element>>> elementPayload;
    std::vector<std::vector<Element>> imageArray, carouselArray;

    elementPayload.insert({"images", imageArray});
    elementPayload.insert({"carousel", carouselArray});

    try { //Check if there are elements at all
        YAML::Node config = YAML::LoadFile(inputFile);
        YAML::Node elements = config["elements"];

        if (!elements) {
            std::cerr << "No elements found in config." << std::endl;
            return elementPayload;
        }

        //Begin parsing of the elements

        for (YAML::const_iterator it = elements.begin(); it != elements.end(); ++it) {
            std::string key = it->first.as<std::string>();
            YAML::Node value = it->second;

            std::string type = value["type"].as<std::string>();
            int id = value["id"].as<int>();

            // IMAGE TYPE
            if (type == "image") {
                //Ensuring required data is all present for adding to the canvas
                if (!value["filepath"] || !value["location"]) {
                    std::cerr << "Missing filepath or location for element: " << key << std::endl;
                    abort();
                }

                std::string filepath = value["filepath"].as<std::string>();
                std::vector<int> locVec = value["location"].as<std::vector<int>>();

                //Checks to ensure location vector is of the expected form + init openCV point
                if ((locVec.size() != 2) || (locVec.at(0) < 0) || (locVec.at(1) < 0)) {
                    std::cerr << "Location for element " << key << " malformed." << std::endl;
                    abort();
                }
                cv::Point loc(locVec.at(0), locVec.at(1));

                //ELEMENT CREATION HERE
                Element elem(filepath, id, loc);


                //Payload is images -> vec of elements. We wrap individual elements in a vec and push it tp the vector.
                std::vector<Element> wrappedElem = {elem};
                elementPayload["images"].push_back(wrappedElem);
            }


            //CAROUSEL
            else if(type == "carousel"){


                //Ensuring required data is all present for adding to the canvas
                if (!value["filepaths"] || !value["location"]) {
                    std::cerr << "Missing filepaths or location for element: " << key << std::endl;
                    abort();
                }

                std::vector<std::string> filepaths = value["filepaths"].as<std::vector<std::string>>();
                std::vector<int> locVec = value["location"].as<std::vector<int>>();

                //Checks to ensure location vector is of the expected form + init openCV point
                if ((locVec.size() != 2) || (locVec.at(0) < 0) || (locVec.at(1) < 0)) {
                    std::cerr << "Location for element " << key << " malformed." << std::endl;
                    abort();
                }
                cv::Point loc(locVec.at(0), locVec.at(1));

                //Create an element for every filepath in the vector and contain them in a vector
                std::vector<Element> carouselArray;
                for(const auto& path : filepaths){
                    Element elem(path, id, loc);
                    carouselArray.push_back(elem);
                }

                elementPayload["carousel"].push_back(carouselArray);

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
