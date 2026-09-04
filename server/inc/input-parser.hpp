#ifndef INPUT_PARSER_HPP
#define INPUT_PARSER_HPP

#include "rtmp.hpp"
#include <map>
#include <opencv2/opencv.hpp>
#include <string>
#include <tuple>
#include <vector>

class Element;
class VirtualCanvas;

void parseInput(VirtualCanvas &vCanvas, std::string &inputFile,
                RTMPServer &rtmpServer);

#endif
