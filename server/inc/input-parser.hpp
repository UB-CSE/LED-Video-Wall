#ifndef INPUT_PARSER_HPP
#define INPUT_PARSER_HPP

#include <string>
#include <tuple>
#include <vector>
#include <map>
#include <opencv2/opencv.hpp>

//Forward declaration instead of full include
class Element;

using ElemVec = std::vector<Element>;
using Payload = std::map<std::string, std::vector<ElemVec>>;

Payload parseInput(const std::string& inputFile);

#endif
