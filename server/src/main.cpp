#include "tcp.hpp"
#include "canvas.h"
#include "client.hpp"
#include "config-parser.hpp"

#include <opencv2/opencv.hpp>

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
#include <vector>

void canvas_debug(VirtualCanvas& vCanvas);

int main() {
    //Create a virtual canvas
    VirtualCanvas vCanvas(cv::Size(8, 32));

    std::vector<Client*> clients_exp;
    try {
        clients_exp = parse_config_throws("config.yaml");
    } catch (std::exception& ex) {
        std::cerr << "Error Parsing config file: " << ex.what() << "\n";
        exit(-1);
    }

    for (Client* c : clients_exp) {
        std::cout << c->to_string() << "\n";
    }

    canvas_debug(vCanvas);

    std::optional<LEDTCPServer> server_opt = create_server(INADDR_ANY, 7070);
    if (!server_opt.has_value()) {
        exit(-1);
    }
    LEDTCPServer server = server_opt.value();

    server.wait_all_join(clients_exp);

    while(1) {
        for (Client* c : clients_exp) {
            std::cout << "Send set_leds to " << c->mac_addr << "\n";
            c->set_leds_all_matrices(vCanvas.getPixelMatrix());
        }
        sleep(3);
    }
}

void canvas_debug(VirtualCanvas& vCanvas) {
    //Create elements (filepath, id, location)
    Element elem1("images/img5x5_1.jpg", 1, cv::Point(0, 0));
    vCanvas.addElementToCanvas(elem1);
}
