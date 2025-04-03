#include <cstdio>
#include <exception>
#include <iostream>
#include <iterator>
#include <optional>
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

    std::optional<ServerConfig> server_config_opt;
    try {
        server_config_opt = parse_config_throws("config.yaml");
    } catch (std::exception& ex) {
        std::cerr << "Error Parsing config file: " << ex.what() << "\n";
        exit(-1);
    }
    ServerConfig server_config = server_config_opt.value();

    std::cout << "Size: " << server_config.canvas_size << "\n";
    VirtualCanvas vCanvas(server_config.canvas_size);

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

    for (Client* c : server_config.clients) {
        std::cout << c->to_string() << "\n";
    }

    std::optional<LEDTCPServer> server_opt =
        create_server(INADDR_ANY, 7070, 7074, server_config.clients);
    if (!server_opt.has_value()) {
        exit(-1);
    }
    LEDTCPServer server = server_opt.value();
    server.start();

    Controller cont(vCanvas,
                    server_config.clients,
                    server,
                    server_config.ns_per_tick,
                    server_config.ns_per_frame);

    while(1) {
        cont.tick_exec();
    }

    return 0;
}
