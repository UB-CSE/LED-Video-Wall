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
#include "rtmp.hpp"
#include "client.hpp"
#include "config-parser.hpp"
#include <vector>
#include "canvas.hpp"
#include "input-parser.hpp"
#include <opencv2/opencv.hpp>
#include "controller.hpp"
#include "command.hpp"
#include <thread>
#include <chrono>
#include <fcntl.h>
#include <unistd.h>  
#include <sys/stat.h>

//Change this flag as needed. Debug mode displays virtual canvas locally per update
#define TMP_CMD "/tmp/led-cmd"

int main(int argc, char* argv[]) {

     //Required for webcam streaming
     setenv("RDMAV_FORK_SAFE", "1", 1);
     setenv("OPENCV_FFMPEG_CAPTURE_OPTIONS", "rtsp_transport;udp", 1);

     ServerConfig server_config;
     std::optional<ServerConfig> server_config_opt;
     try {
         server_config_opt = parse_config_throws("config.yaml");
     } catch (std::exception& ex) {
         std::cerr << "Error Parsing config file: " << ex.what() << "\n";
         exit(-1);
     }
     server_config = server_config_opt.value();
 
     std::map <std::string, std::vector<std::vector<Element>>> elements;
 
     VirtualCanvas vCanvas(server_config.canvas_size);
     vCanvas.pixelMatrix = cv::Mat::zeros(vCanvas.dim, CV_8UC3);
 
     std::string inputFilePath;
     bool debug_mode = true;
     if (argc >= 2) {
         inputFilePath = std::string(argv[1]);
         if ( (argc == 3) && (std::string(argv[2]) == "--prod") ) {
            debug_mode = 0;
         };
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

     RTMPServer rtmp_server;
 
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
                     server_config.ns_per_frame);
    


    //Setup for pipes
    unlink(TMP_CMD); //Destroys the existing pipe - dont want leftover commands if any
    if (mkfifo(TMP_CMD, 0666) == -1 && errno != EEXIST) {std::cerr << "mkfifo failed: " << strerror(errno) << "\n";return 1;} //Creates a fifo style pipe
    int pipe = open(TMP_CMD, O_RDONLY | O_NONBLOCK); //Opens the pipe for reading only
    if (pipe < 0) {std::cerr << "open failed: " << strerror(errno) << "\n";return 1;}

    bool isPaused = false;
    char buf[256];
    std::cout << "\nWrite your command to " << TMP_CMD << std::endl << "Example: `echo \"move 5 10 10 > " << TMP_CMD << "\'" << std::endl <<  "Available Commands : \n- pause\n- resume\n- quit\n- move <ElementID> <x-coord> <y-coord>\n- add <type> <ElementID> <x-coord> <y-coord>\n- remove <ElementID>\n";
     while(1) {

        /*
        ======================================================================================
        Command line shenanigans: Using Pipes now:

        From another process, you now enter commands by writing to the FIFO file in "TMP_CMD"
        By default, it is "/tmp/led-cmd". 
        
        For example, open another terminal, and if I want to move an element, I would do:

        `echo "move 5 10 10" > /tmp/led-cmd`

        Available Commands : 
        - pause
        - resume
        - quit
        - move <ElementID> <x-coord> <y-coord

        ======================================================================================
        */

       ssize_t n = read(pipe, buf, sizeof(buf) - 1);
       if (n > 0) {
            buf[n] = '\0';
            //Remove whitespaces
            std::string line(buf);
            line.erase(line.find_last_not_of(" \t\r\n") + 1);

            if (!line.empty()) {
                int status = processCommand(vCanvas, line, isPaused);
                if (status == 1) {goto EXIT_PROGRAM;}
            }
        }

        /*
        =============================================
                        Continue
        =============================================
        */

        if (!isPaused) {
            cont.frame_exec(debug_mode);
        }
     }

    EXIT_PROGRAM:
    close(pipe);
    unlink(TMP_CMD);
    return 0;
}
