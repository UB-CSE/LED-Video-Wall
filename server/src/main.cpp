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
#include <signal.h>
#include <filesystem>
#include <cef_app.h>
#include <cef_command_line.h>

//Change this flag as needed. Debug mode displays virtual canvas locally per update
#define TMP_CMD "/tmp/led-cmd"

static volatile sig_atomic_t stop_signal = 0;

static void signal_handler(int signum) {
  if (stop_signal) {
    std::cout << "Received second signal, exiting immediately...\n";
    exit(1);
  }

  stop_signal = 1;
  std::cout << "Received signal, exiting now...\n";
}

static std::string inputFilePath;
static bool debug_mode = true;
static int ledvwPort = 7070;
static int rtmpPort = 1935;
static std::string rtmpCertPath, rtmpKeyPath;

static bool validate_port(int port) {
    if (port < 1 || port > 65535) {
        std::cerr << "Error: Port must be a valid port number (0-65535).\n";
        return false;
    }
    if (port < 1024) {
        std::cerr << "Error: System ports (0-1023) are reserved.\n";
        return false;
    }
    return true;
}

static bool handle_command_line() {
    CefRefPtr<CefCommandLine> cmdLine = CefCommandLine::GetGlobalCommandLine();

    std::map<CefString, CefString> switches;
    cmdLine->GetSwitches(switches);
    std::vector<CefString> arguments;
    cmdLine->GetArguments(arguments);

    if (arguments.empty()) {
        std::cerr << "Error, no input file specified!" << "\n";
        return false;
    }
    inputFilePath = arguments.front().ToString();

    if (switches.contains("prod")) {
       debug_mode = false;
    }

    if (switches.contains("rtmp-tls-cert")) {
       rtmpCertPath = switches.at("rtmp-tls-cert").ToString();
    }
    if (switches.contains("rtmp-tls-key")) {
       rtmpKeyPath = switches.at("rtmp-tls-key").ToString();
    }
    if (rtmpCertPath.empty() != rtmpKeyPath.empty()) {
       std::cerr << "Error: Both --rtmp-tls-cert and --rtmp-tls-key must be provided together to enable RTMP TLS.\n";
       return false;
    }

    if (switches.contains("ledvw-port")) {
        try {
            ledvwPort = std::stoi(switches.at("ledvw-port").ToString());
        } catch (const std::exception& ex) {
            std::cerr << "Error: LEDVW port is not a valid integer.\n";
            return false;
        }
    }
    if (switches.contains("rtmp-port")) {
        try {
            rtmpPort = std::stoi(switches.at("rtmp-port").ToString());
        } catch (const std::exception& ex) {
            std::cerr << "Error: RTMP port is not a valid integer.\n";
            return false;
        }
    }

    if (!validate_port(ledvwPort) || !validate_port(rtmpPort)) {
        return false;
    }

    if(ledvwPort == rtmpPort) {
        std::cerr << "Error: LEDVW port and RTMP port cannot be the same.\n";
        return false;
    }

    return true;
}

int main(int argc, char* argv[]) {
    CefMainArgs args(argc, argv);

    // Execute the sub-process logic, if any. This will either return immediately for the browser
    // process or block until the sub-process should exit.
    int result = CefExecuteProcess(args, nullptr, nullptr);
    if (result >= 0) {
        // The sub-process terminated, exit now.
        return result;
    }

    // Initialize CEF in the main process.
    CefSettings settings;
    settings.windowless_rendering_enabled = true;
    std::filesystem::path cachePath = std::filesystem::current_path() / "cef-cache";
    CefString(&settings.cache_path).FromString(cachePath.string());
    if (!CefInitialize(args, settings, nullptr, nullptr)) {
      exit(-1);
    }

    signal(SIGINT, signal_handler); // Ctrl+C
    signal(SIGTERM, signal_handler); // Web server sends TERM signal to shutdown

    //Required for webcam streaming
    setenv("RDMAV_FORK_SAFE", "1", 1);
    setenv("OPENCV_FFMPEG_CAPTURE_OPTIONS", "rtsp_transport;udp", 1);

    ServerConfig server_config;
    std::optional<ServerConfig> server_config_opt;
    try {
        server_config_opt = parse_config_throws("config.yaml");
    } catch (std::exception& ex) {
        std::cerr << "Error Parsing config file: " << ex.what() << "\n";
        CefShutdown();
        exit(1);
    }
    server_config = server_config_opt.value();

    VirtualCanvas vCanvas(server_config.canvas_size);
    vCanvas.pixelMatrix = cv::Mat::zeros(vCanvas.dim, CV_8UC3);

    if (!handle_command_line()) {
        CefShutdown();
        exit(1);
    }

    RTMPServer rtmpServer(rtmpPort, "0.0.0.0", rtmpCertPath, rtmpKeyPath);

    try {
        parseInput(vCanvas, inputFilePath, rtmpServer);
    } catch (std::exception& ex) {
        std::cerr << "Error Parsing image input file ("
                  << inputFilePath << "):"
                  << ex.what() << "\n";
        CefShutdown();
        exit(1);
    }


    std::shared_ptr<LEDTCPServer> server =
        create_server(INADDR_ANY, ledvwPort, server_config.clients);
    if (!server) {
        CefShutdown();
        exit(1);
    }
    server->start();

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
    while(!stop_signal) {
        CefDoMessageLoopWork();

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

    CefShutdown();

    return 0;
}
