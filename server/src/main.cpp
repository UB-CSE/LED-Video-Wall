#include "canvas.hpp"
#include "client.hpp"
#include "command.hpp"
#include "controller.hpp"
#include "logger.hpp"
#include "matrix-config.hpp"
#include "rtmp.hpp"
#include "tcp.hpp"
#include <OptionParser.hpp>
#include <cef_app.h>
#include <cef_command_line.h>
#include <chrono>
#include <cstdio>
#include <exception>
#include <fcntl.h>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <netdb.h>
#include <netinet/in.h>
#include <opencv2/opencv.hpp>
#include <optional>
#include <signal.h>
#include <spdlog/spdlog.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h> // for close
#include <unistd.h>
#include <vector>

// Change this flag as needed. Debug mode displays virtual canvas locally per
// update
#define TMP_CMD "/tmp/led-cmd"

namespace {

volatile sig_atomic_t stop_signal = 0;

void signalHandler(int signum) {
  if (stop_signal) {
    spdlog::info("received second signal, exiting immediately...");
    exit(1);
  }

  stop_signal = 1;
  spdlog::info("received signal, exiting now...");
}

enum Options {
  Help,
  Prod,
  Interactive,
  CanvasConfig,
  RTMP_TLSCert,
  RTMP_TLSKey
};

const CommandLine::Option cmdLineOptions[] = {
    {
        .ID = Options::Help,
        .Names = {"help", "h"},
        .Help = "Print this help message",
    },
    {
        .ID = Options::Prod,
        .Names = {"prod", "production"},
        .Help = "Run in production mode (disable debugging additions)",
    },
    {
        .ID = Options::Interactive,
        .Names = {"interactive", "i"},
        .Help = "Show a command prompt for interacting with the canvas",
    },

    {
        .ID = Options::CanvasConfig,
        .Names = {"canvas-config", "canvas"},
        .RequiresValue = true,
        .ValueName = "config-file",
        .Help = "Load a canvas config YAML file at start-up",
    },
    {
        .ID = Options::RTMP_TLSCert,
        .Names = {"rtmp-tls-cert"},
        .RequiresValue = true,
        .ValueName = "cert-file",
        .Help = "Set the RTMP TLS Certificate (use with --rtmp-tls-key)",
    },
    {
        .ID = Options::RTMP_TLSKey,
        .Names = {"rtmp-tls-key"},
        .RequiresValue = true,
        .ValueName = "key-file",
        .Help = "Set the RTMP TLS Key (use with --rtmp-tls-cert)",
    },
};

std::filesystem::path matrixConfigFilePath;
std::filesystem::path canvasConfigFilePath;

// Disable debug window if true
bool prodMode = true;
// Show command prompt if true, otherwise logs
bool interactiveMode = false;

std::string rtmpCertPath, rtmpKeyPath;

bool handleCommandLine(int argc, char **argv) {
  auto cmdLine =
      CommandLine::Parse(argc, const_cast<const char **>(argv), cmdLineOptions);
  if (!cmdLine) {
    return false;
  }

  if (cmdLine->Options.contains(Options::Help)) {
    CommandLine::PrintUsage(argv[0], cmdLineOptions);
    return false;
  }

  prodMode = cmdLine->Options.contains(Options::Prod);
  interactiveMode = cmdLine->Options.contains(Options::Interactive);

  if (cmdLine->Options.contains(Options::RTMP_TLSCert)) {
    rtmpCertPath = cmdLine->Options.at(Options::RTMP_TLSCert);
  }
  if (cmdLine->Options.contains(Options::RTMP_TLSKey)) {
    rtmpKeyPath = cmdLine->Options.at(Options::RTMP_TLSKey);
  }
  if (rtmpCertPath.empty() != rtmpKeyPath.empty()) {
    spdlog::error("both --rtmp-tls-cert and --rtmp-tls-key are required to "
                  "enable RTMP TLS");
    return false;
  }

  if (cmdLine->Arguments.empty()) {
    spdlog::error("no matrix configuration file specified");
    return false;
  }

  matrixConfigFilePath = cmdLine->Arguments.front();
  if (!std::filesystem::exists(matrixConfigFilePath) ||
      matrixConfigFilePath.extension() != ".yaml") {
    spdlog::error("invalid matrix configuration file `{}'",
                  matrixConfigFilePath.string());
    return false;
  }

  if (cmdLine->Options.contains(Options::CanvasConfig)) {
    canvasConfigFilePath = cmdLine->Options[Options::CanvasConfig];
  }

  return true;
}

class MyApp : public CefApp {
public:
  MyApp() = default;

  void OnBeforeCommandLineProcessing(
      const CefString &process_type,
      CefRefPtr<CefCommandLine> command_line) override {
    command_line->AppendSwitchWithValue("ozone-platform", "headless");
  }

  IMPLEMENT_REFCOUNTING(MyApp);
};

 } // namespace

int main(int argc, char *argv[]) {
  CefMainArgs args(argc, argv);

  CefRefPtr app(new MyApp);

  /**
   * Execute the CEF sub-process logic, if any. This will either return
   * immediately for the main server process or block until the CEF sub-process
   * should exit.
   *
   * IMPORTANT: Perform all server initialization AFTER this so that things do
   * not get initialized for every sub-process!
   */
  int result = CefExecuteProcess(args, app.get(), nullptr);
  if (result >= 0) {
    // The sub-process terminated, exit now.
    return result;
  }

  if (!handleCommandLine(argc, argv)) {
    return 1;
  }

  // Initialize CEF in the main process.
  CefSettings settings;
  settings.windowless_rendering_enabled = true;

  const std::filesystem::path logsDirectory =
      std::filesystem::current_path() / "logs";

  if (prodMode || interactiveMode) {
    const bool withStdout = (interactiveMode == false);

    std::filesystem::path mainLogFilePath =
        InitializeLogging(withStdout, logsDirectory, DefaultMaxLogFiles);

    // We cannot capture CEF log messages and redirect them to our main log
    // file, so CEF gets its own file.
    std::filesystem::path cefLogFilePath =
        MakeLogFilePath(logsDirectory, "CEF");
    CleanUpLogsDirectory(DefaultMaxLogFiles, logsDirectory, "CEF");
    CefString(&settings.log_file).FromString(cefLogFilePath.string());

    // CEF logging is silly... even though you tell it to log to file, it will
    // still log errors to stderr anyways! So when in interactive mode we need
    // to just disable all logging so that it does not interfere with the
    // command prompt.
    if (!withStdout) {
      settings.log_severity = LOGSEVERITY_DISABLE;
    }

    if (!withStdout) {
      // Let the user know where logs are.
      std::cout << "Logging to files:\n";
      std::cout << "LEDVW: " << mainLogFilePath.string() << '\n';
      std::cout << "  CEF: " << cefLogFilePath.string() << '\n';
    } else {
      spdlog::info("logging to files:\n\tLEDVW: {}\n\t  CEF: {}",
                   mainLogFilePath.string(), cefLogFilePath.string());
    }
  } else {
    (void)InitializeLogging();
  }

  // Each instance of the server needs its own cache folder.
  std::filesystem::path cefCacheDirectoryPath =
      std::filesystem::current_path() / "cef-caches" /
      ("cef-cache-" + matrixConfigFilePath.filename().string());

  CefString(&settings.cache_path).FromString(cefCacheDirectoryPath.string());

  if (!CefInitialize(args, settings, app.get(), nullptr)) {
    exit(1);
  }

  signal(SIGINT, signalHandler);  // Ctrl+C
  signal(SIGTERM, signalHandler); // Web server sends TERM signal to shutdown

  // Required for webcam streaming
  setenv("RDMAV_FORK_SAFE", "1", 1);
  setenv("OPENCV_FFMPEG_CAPTURE_OPTIONS", "rtsp_transport;udp", 1);

  MatrixConfig matrixConfig;
  if (!matrixConfig.load(matrixConfigFilePath)) {
    CefShutdown();
    exit(1);
  }

  VirtualCanvas vCanvas(matrixConfig.canvas_size);

  RTMPServer rtmpServer(matrixConfig.rtmpPort, "0.0.0.0", rtmpCertPath,
                        rtmpKeyPath);

  if (!canvasConfigFilePath.empty()) {
    if (!vCanvas.loadElementConfig(canvasConfigFilePath, rtmpServer)) {
      CefShutdown();
      exit(1);
    }
  }

  std::shared_ptr<LEDTCPServer> server = create_server(
      INADDR_ANY, matrixConfig.ledvwPort, matrixConfig.clients,
      matrixConfig.brightness_percent, matrixConfig.image_encoding);
  if (!server) {
    CefShutdown();
    exit(1);
  }
  server->start();

  Controller cont(vCanvas, server, matrixConfig.ns_per_frame);

  // Each instance gets its own command pipe based on its ledvw port.
  const std::string cmd_pipe =
      std::string(TMP_CMD) + "-" + std::to_string(matrixConfig.ledvwPort);

  // Setup for pipes
  unlink(cmd_pipe.c_str()); // Destroys the existing pipe - dont want leftover
                            // commands if any
  if (mkfifo(cmd_pipe.c_str(), 0666) == -1 && errno != EEXIST) {
    spdlog::error("mkfifo() failed: {}", strerror(errno));
    return 1;
  } // Creates a fifo style pipe
  int pipe = open(cmd_pipe.c_str(),
                  O_RDONLY | O_NONBLOCK); // Opens the pipe for reading only
  if (pipe < 0) {
    spdlog::error("open() failed: {}", strerror(errno));
    return 1;
  }

  bool isPaused = false;
  char buf[256];
  while (!stop_signal) {
    CefDoMessageLoopWork();

    /*
    ======================================================================================
    Command line shenanigans: Using Pipes now:

    From another process, you now enter commands by writing to the FIFO file in
    "TMP_CMD" By default, it is "/tmp/led-cmd".

    For example, open another terminal, and if I want to move an element, I
    would do:

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
      // Remove whitespaces
      std::string line(buf);
      line.erase(line.find_last_not_of(" \t\r\n") + 1);

      if (!line.empty()) {
        bool isRunning = true;
        nlohmann::json commandResult =
            ProcessCommand(vCanvas, line, isPaused, isRunning);
        if (!commandResult.empty()) { // Command was invoked.
          spdlog::info("[Command] result:\n{}", commandResult.dump(2));
        }

        if (!isRunning) {
          goto EXIT_PROGRAM;
        }
      }
    }

    /*
    =============================================
                    Continue
    =============================================
    */

    if (!isPaused) {
      cont.frame_exec(!prodMode);
    }
  }

EXIT_PROGRAM:
  close(pipe);
  unlink(cmd_pipe.c_str());

  CefShutdown();

  return 0;
}
