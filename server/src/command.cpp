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


int processCommand(VirtualCanvas& vCanvas, std::string& line) {
    std::istringstream iss(line);
    std::string command;
    iss >> command; //Gets the first word

    if (command == "quit") {
        return 1;
    }

    if (command == "pause") {
        std::cout << "Paused.. Use <resume> to continue\n";
        std::string subcmd;
        while (true) {
            std::getline(std::cin, subcmd);
            if (subcmd == "resume") break;
            processCommand(vCanvas, subcmd);  // Allow commands while paused
        }
        return 0;
    }

    if (command == "move") {
        int id, x, y;
        if ((!(iss >> id >> x >> y)) || (x < 0) || (y < 0)) {
            std::cerr << "Invalid move command: expected 3 positive arguments\n- move <ElementID> <x-coord> <y-coord>\n";
            return -1; 
        }

        cv::Point loc(x, y);
        vCanvas.moveElement(id, loc);
        return 0;
    }

    std::cout << "Unknown command: " << command << "\n";
    printf("\n Available Commands : \n- pause\n- resume\n- quit\n- move <ElementID> <x-coord> <y-coord>\n");
    return 0;
}