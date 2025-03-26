#include "controller.hpp"
#include "tcp.hpp"

Controller::Controller(VirtualCanvas& canvas,
                       std::vector<Client*> clients,
                       LEDTCPServer tcp_server)
    : canvas(canvas),
      clients(clients),
      tcp_server(tcp_server),
      client_conn_info(tcp_server.conn_info)
{}

void Controller::set_leds_all() {
    std::vector<std::pair<const Client*, int>> conns;
    this->client_conn_info->getAllConnected(conns);
    for (auto it : conns) {
        for (MatricesConnection conn : it.first->mat_connections) {
            uint8_t pin = conn.pin;
            for (LEDMatrix* mat : conn.matrices) {
                this->tcp_server.set_leds(it.first,
                                          it.second,
                                          this->canvas.getPixelMatrix(),
                                          mat,
                                          pin,
                                          8);
            }
        }
    }
}
