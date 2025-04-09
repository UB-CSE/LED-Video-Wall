#ifndef INPUT_PARSER_HPP
#define INPUT_PARSER_HPP

#include <string>
#include <tuple>
#include <vector>
#include <map>
#include <opencv2/opencv.hpp>

class Element;
class VirtualCanvas;

void parseInput(VirtualCanvas& vCanvas,  std::string& inputFile);

#endif
