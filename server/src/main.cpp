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

    std::map <std::string, std::vector<std::vector<Element>>> elements;
    try {
        elements = parseInput(inputFilePath);
    } catch (std::exception& ex) {
        std::cerr << "Error Parsing image input file ("
                  << inputFilePath << "):"
                  << ex.what() << "\n";
        exit(-1);
    }
    vCanvas.addPayloadToCanvas(elements);

    for (Client* c : clients_exp.first) {
        std::cout << c->to_string() << "\n";
    }

    std::optional<LEDTCPServer> server_opt =
        create_server(INADDR_ANY, 7070, 7074, clients_exp.first);
    if (!server_opt.has_value()) {
        exit(-1);
    }
    LEDTCPServer server = server_opt.value();
    server.start();

    uint64 ns_per_tick = 010'000'000; // 100 tps
    uint64 ns_per_frame = 200'000'000; // 5 fps

    Controller cont(vCanvas, clients_exp.first, server, ns_per_tick, ns_per_frame);

    while(1) {
        cont.tick_exec();
    }

    return 0;
}
