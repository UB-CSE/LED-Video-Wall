#ifndef COMMAND_HPP
#define COMMAND_HPP

#include "canvas.h"

bool inputAvailable();
int processCommand(VirtualCanvas& vCanvas, const std::string& line, bool& isPaused);

#endif