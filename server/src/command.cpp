#include <iostream>
#include <unistd.h>
#include <sys/select.h>
#include <string>
#include "canvas.h"
#include "command.hpp"

/*
inputAvailable is boilerplate code from my past projects. This is intended to allow
stdin to be read afterwards without blocking from waiting for input.

This checks to see if stdin has an input waiting. If there is, this is true. If
there is not, this resolves to false

*/

bool inputAvailable() {
    struct timeval tv = {0, 0};  // zero timeout = check instantly
    fd_set fds;
    FD_ZERO(&fds);               // Clear the set
    FD_SET(STDIN_FILENO, &fds);  // Add stdin (fd 0)

    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}


int processCommand(VirtualCanvas& vCanvas, const std::string& line, bool& isPaused) {
    std::istringstream iss(line);
    std::string cmd;
    iss >> cmd;
    
    if (cmd == "quit") {
        return 1;
    }
    if (cmd == "pause") {
        isPaused = true;
        std::cout << "Paused.  Send <resume> to continue.\n";
        return 0;
    }
    if (cmd == "resume") {
        isPaused = false;
        std::cout << "Resumed.\n";
        return 0;
    }
    if (cmd == "move") {
        int id, x, y;
        if (!(iss >> id >> x >> y) || x < 0 || y < 0) {
            std::cerr << "Invalid move. Usage:\n  move <ElementID> <x> <y>\n";
        } else {
            vCanvas.moveElement(id, cv::Point(x, y));
        }
        return 0;
    }
    std::cout << "Unknown command: " << cmd << "\n"
                 "Available: pause, resume, quit, move <id> <x> <y>\n";
    return 0;
}