#ifndef COMMAND_HPP
#define COMMAND_HPP

#include "canvas.hpp"

bool inputAvailable();
int processCommand(VirtualCanvas& vCanvas, const std::string& line, bool& isPaused);

#endif
