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

    for (Client* c : clients_exp.first) {
        std::cout << c->to_string() << "\n";
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

    Element elem1("images/img5x5_1.jpg", 74829, cv::Point(0, 0));
    vCanvas.addElementToCanvas(elem1);

    std::optional<LEDTCPServer> server_opt = create_server(INADDR_ANY, 7070, 7074);
    if (!server_opt.has_value()) {
        exit(-1);
    }
    LEDTCPServer server = server_opt.value();

    server.wait_all_join(clients_exp.first);

    for (Client* c : clients_exp.first) {
        c->set_leds_all_matrices(vCanvas.getPixelMatrix());
    }

    int x = 0;
    int y = 0;
    int dx = 1;
    int dy = 1;
    while(1) {
        for (Client* c : clients_exp.first) {
            c->set_leds_all_matrices(vCanvas.getPixelMatrix());
        }
        vCanvas.removeElementFromCanvas(elem1);
        if (x >= 11) {
            dx = -1;
        } else if (x <= 0) {
            dx = 1;
        }
        if (y >= 26) {
            dy = -1;
        } else if (y <= 0) {
            dy = 1;
        }
        x += dx;
        y += dy;
        elem1.setLocation(cv::Point(x, y));
        vCanvas.addElementToCanvas(elem1);
        // usleep(33333); // ~30 fps
        // usleep(100000); // 10 fps
        // usleep(200000); // 5 fps
        usleep(250000); // 4 fps
    }

    return 0;
}
