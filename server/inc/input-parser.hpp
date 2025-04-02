#ifndef INPUT_PARSER_HPP
#define INPUT_PARSER_HPP

#include <string>
#include <tuple>
#include <vector>
#include <map>
#include <opencv2/opencv.hpp>

//Forward declaration instead of full include
class Element;

using ElemTuple = std::tuple<int, int, std::vector<Element>>;
using Payload = std::map<std::string, std::vector<ElemTuple>>;

Payload parseInput(const std::string& inputFile);

#endif
