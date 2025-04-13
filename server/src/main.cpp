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
#include <thread>
#include <chrono>

#include <mpi.h>
#define MASTER_PROCESSOR 0
#define CANVAS_PROCESSOR 1

int main(int argc, char* argv[]) {

    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    //MPI 3's RMA setup. One window object and one pointer to walk through the buffer
    MPI_Win win;
    uchar* win_base_ptr = nullptr;

    ServerConfig server_config;
    int canvas_size[2];
    if(rank == MASTER_PROCESSOR) {
        std::optional<ServerConfig> server_config_opt;
        try {
            server_config_opt = parse_config_throws("config.yaml");
        } catch (std::exception& ex) {
            std::cerr << "Error Parsing config file: " << ex.what() << "\n";
            exit(-1);
        }
        server_config = server_config_opt.value();
        canvas_size[0] = server_config.canvas_size.width;
        canvas_size[1] = server_config.canvas_size.height;
    }
    MPI_Bcast(&canvas_size, 2, MPI_INT, MASTER_PROCESSOR, MPI_COMM_WORLD);

    std::map <std::string, std::vector<std::vector<Element>>> elements;

    VirtualCanvas vCanvas;
    if(rank == CANVAS_PROCESSOR) {
        vCanvas.dim = cv::Size(canvas_size[0], canvas_size[1]);
        vCanvas.pixelMatrix = cv::Mat::zeros(vCanvas.dim, CV_8UC3);

        std::string inputFilePath;
        if (argc == 2) {
            inputFilePath = std::string(argv[1]);
        } else {
            std::cerr << "Error, no image input file specified!" << "\n";
            exit(-1);
        }

        try {
            parseInput(vCanvas, inputFilePath);
        } catch (std::exception& ex) {
            std::cerr << "Error Parsing image input file ("
                      << inputFilePath << "):"
                      << ex.what() << "\n";
            exit(-1);
        }

        win_base_ptr = vCanvas.pixelMatrix.data;
        MPI_Win_create(win_base_ptr, vCanvas.pixelMatrix.total() * vCanvas.pixelMatrix.elemSize(), sizeof(uchar), MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    }

    if(rank == MASTER_PROCESSOR) {
        MPI_Win_create(nullptr, 0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &win);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if(rank == MASTER_PROCESSOR) {
        std::optional<LEDTCPServer> server_opt =
        create_server(INADDR_ANY, 7070, 7074, server_config.clients);
        if (!server_opt.has_value()) {
            exit(-1);
        }
        LEDTCPServer server = server_opt.value();
        server.start();

        Controller cont(win,
                        server_config.canvas_size,
                        server_config.clients,
                        server,
                        server_config.ns_per_frame);

        while(1) {
            cont.frame_exec();
        }
    }

    if (rank == CANVAS_PROCESSOR) {
        while (true) {
            MPI_Win_lock(MPI_LOCK_EXCLUSIVE, CANVAS_PROCESSOR, 0, win);
            std::cout << "[CANVAS_PROCESSOR] Updating frame...\n";
            vCanvas.pushToCanvas();
            MPI_Win_unlock(CANVAS_PROCESSOR, win);

            //cv::imshow("Display Cats", vCanvas.getPixelMatrix());
            //cv::waitKey(1);

            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    MPI_Win_free(&win);
    MPI_Finalize();

    return 0;
}
