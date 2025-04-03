#ifndef INPUT_PARSER_HPP
#define INPUT_PARSER_HPP

#include <string>
#include <tuple>
#include <vector>
#include <map>
#include <opencv2/opencv.hpp>

class Element;

using ElemVec = std::vector<Element>;
using Payload = std::map<std::string, std::vector<ElemVec>>;

Payload parseInput(const std::string& inputFile, int64_t tick_rate);

#endif
