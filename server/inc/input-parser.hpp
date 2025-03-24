#ifndef INPUT_PARSER_HPP
#define INPUT_PARSER_HPP

#include <string>
#include <unordered_map>
#include "canvas.h"

std::map <std::string, std::vector<std::vector<Element>>> parseInput(const std::string inputFile);

#endif
