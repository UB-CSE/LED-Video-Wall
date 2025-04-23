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




void parseInput(VirtualCanvas& vCanvas,  std::string& inputFile) {

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

                Element * elem = new VideoElement(webcamNum, id, loc, frameRate);

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

