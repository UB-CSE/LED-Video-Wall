#include "tcp.hpp"
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <iterator>
#include <string.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

int create_server_socket(uint32_t addr, uint16_t port) {
    struct protoent* protocol_entry = getprotobyname("tcp");
    const int tcp_protocol_num = protocol_entry->p_proto;
    
    int server_socket = socket(AF_INET, SOCK_STREAM, tcp_protocol_num);
    if (server_socket == -1) {
        std::cerr << "Bad socket!\n";
        return -1;
    }

    struct sockaddr_in s_addr;
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(port);
    s_addr.sin_addr.s_addr = addr;

    int res = bind(server_socket, (struct sockaddr *)&s_addr, sizeof(s_addr));
    if (res == -1) {
        std::cerr << "Failed bind: " << strerror(errno) << "\n";
        return -1;
    }

    return server_socket;
}
