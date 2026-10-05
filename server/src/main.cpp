#include "canvas.hpp"
#include "client.hpp"
#include "command.hpp"
#include "controller.hpp"
#include "logger.hpp"
#include "matrix-config.hpp"
#include "rtmp.hpp"
#include "tcp.hpp"
#include "unix-socket-msg-channel-tests.hpp"
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
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h> // for close
#include <unistd.h>
#include <vector>

namespace {

volatile sig_atomic_t stop_signal = 0;

void signalHandler(int signum) {
  if (stop_signal) {
    std::cout << "\nreceived second signal, exiting immediately...\n";
    exit(1);
  }

  stop_signal = 1;
  std::cout << "\nreceived signal, exiting now...\n";
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

  const std::string serverInstanceName = matrixConfigFilePath.stem().string();

  // Each instance of the server needs its own cache folder.
  std::filesystem::path cefCacheDirectoryPath =
      std::filesystem::current_path() / "cef-caches" /
      ("cef-cache-" + serverInstanceName);

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

  // Each instance gets its own command unix socket.
  const std::filesystem::path commandUnixSocketPath =
      std::format("/tmp/ledvw-cmd-{}", serverInstanceName);

  UnixSocketCommandSource unixSocketCommandSource(commandUnixSocketPath);

  spdlog::info("server {} started successfully", serverInstanceName);

  if (interactiveMode) {
    std::cout << "\nRun canvas commands here. Run the `help' command to see a "
                 "list of all commands\n";

    PromptCommandSource::get().setPromptName(serverInstanceName);
    PromptCommandSource::get().activate();
  }

  std::vector<CommandSource *> commandSources = {&unixSocketCommandSource,
                                                 &PromptCommandSource::get()};

  bool isPaused = false;
  while (!stop_signal) {
    CefDoMessageLoopWork();

    for (CommandSource *cmdSource : commandSources) {
      cmdSource->process();
    }

    std::string cmdString;
    CommandSource *cmdSource = nullptr;

    for (auto it = commandSources.begin(); it != commandSources.end(); ++it) {
      cmdSource = *it;
      cmdString = cmdSource->consumeLatestCommand();
      if (!cmdString.empty()) {
        break;
      }
    }

    if (!cmdString.empty() && cmdSource != nullptr) {
      std::shared_ptr<spdlog::logger> cmdLogger =
          cmdSource->getCommandOutputLogger();

      bool isRunning = true;
      nlohmann::json commandResult =
          ProcessCommand(vCanvas, cmdString, isPaused, isRunning, cmdLogger);
      if (!commandResult.empty()) { // Command was invoked.
        cmdLogger->info("result:\n{}", commandResult.dump(2));
        cmdSource->handleResponse(commandResult);
      }

      if (!isRunning) {
        break;
      }

      if (interactiveMode) {
        // Re-activate the command prompt
        PromptCommandSource::get().activate();
      }
    }

    if (!isPaused) {
      cont.frame_exec(!prodMode);
    }
  }

  CefShutdown();
  return 0;
}
