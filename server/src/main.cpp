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

void canvas_debug(VirtualCanvas& vCanvas, std::string inputFilePath);

int main() {


    /*
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
    
    
    
    */
    std::string inputFilePath = "input.yaml";
    VirtualCanvas vCanvas(cv::Size(2000, 2000));
    canvas_debug(vCanvas, inputFilePath);
    return 0;
}

void canvas_debug(VirtualCanvas& vCanvas, std::string inputFilePath) {

    std::map <std::string, std::vector<std::vector<Element>>> elementsPayload = parseInput(inputFilePath);
    vCanvas.addPayloadToCanvas(elementsPayload);

    cv::imshow("Display Cats", vCanvas.getPixelMatrix());
    cv::waitKey(0);


    /*
    //Create elements (filepath, id, location)
    Element elem1("images/img.jpg", 1, cv::Point(0, 0));
    vCanvas.addElementToCanvas(elem1);

    cv::imshow("Display Cat 1", vCanvas.getPixelMatrix());
    cv::waitKey(0);

    Element elem2("images/img2.jpg", 1, cv::Point(1700, 69));
    vCanvas.addElementToCanvas(elem2);

    cv::imshow("Display Cat 2", vCanvas.getPixelMatrix());
    cv::waitKey(0);
    
    
    */

    
}
