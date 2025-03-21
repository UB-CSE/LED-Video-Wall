#include <cstdio>
#include <exception>
#include <iostream>
#include <iterator>
#include <string.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h> // for close
#include "tcp.hpp"
#include "client.hpp"
#include "config-parser.hpp"
#include <vector>
#include "canvas.h"
#include "input-parser.hpp"
#include <opencv2/opencv.hpp>
#include "controller.hpp"

int main(int argc, char* argv[]) {
    std::string inputFilePath;
    if (argc == 2) {
        inputFilePath = std::string(argv[1]);
    } else {
        std::cerr << "Error, no image input file specified!" << "\n";
        exit(-1);
    }

    std::pair<std::vector<Client*>, cv::Size> clients_exp;
    try {
        clients_exp = parse_config_throws("config.yaml");
    } catch (std::exception& ex) {
        std::cerr << "Error Parsing config file: " << ex.what() << "\n";
        exit(-1);
    }

    std::cout << "Size: " << clients_exp.second << "\n";
    VirtualCanvas vCanvas(clients_exp.second);

    std::vector<Element> elementsVec;
    try {
        elementsVec = parseInput(inputFilePath);
    } catch (std::exception& ex) {
        std::cerr << "Error Parsing image input file ("
                  << inputFilePath << "):"
                  << ex.what() << "\n";
        exit(-1);
    }
    vCanvas.addElementVecToCanvas(elementsVec);

    for (Client* c : clients_exp.first) {
        std::cout << c->to_string() << "\n";
    }

    Element elem1("images/img5x5_1.jpg", 1, cv::Point(0, 0));
    vCanvas.addElementToCanvas(elem1);

    std::optional<LEDTCPServer> server_opt =
        create_server(INADDR_ANY, 7070, 7074, clients_exp.first);
    if (!server_opt.has_value()) {
        exit(-1);
    }
    LEDTCPServer server = server_opt.value();
    server.start();
    Controller cont(vCanvas, clients_exp.first, server);

    int x = 0;
    int y = 0;
    int dx = 1;
    int dy = 1;
    int max_x = clients_exp.second.width;
    int max_y = clients_exp.second.height;
    while(1) {
        std::cout << "loop\n";
        cont.canvas.removeElementFromCanvas(elem1);
        cont.set_leds_all();
        if (x >= max_x - 5) {
            dx = -1;
        } else if (x <= 0) {
            dx = 1;
        }
        if (y >= max_y - 5) {
            dy = -1;
        } else if (y <= 0) {
            dy = 1;
        }
        x += dx;
        y += dy;
        elem1.setLocation(cv::Point(x, y));
        cont.canvas.addElementToCanvas(elem1);
        // usleep(25000); // 40 fps
        // usleep(33333); // ~30 fps
        // usleep(50000); // 20 fps
        usleep(100000); // 10 fps
        // usleep(200000); // 5 fps
        // usleep(250000); // 4 fps
    }

    return 0;
}
