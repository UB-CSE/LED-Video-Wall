#include "command.hpp"
#include "canvas.hpp"
#include "text-render.hpp"
#include <cctype>
#include <optional>
#include <readline/history.h>
#include <readline/readline.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <string>
#include <sys/types.h>

#pragma region Command Argument Parsing

std::optional<std::string_view>
CommandParser::NextToken(std::string_view &inputStr) {
  const char *input = inputStr.data();
  if (!input) {
    return std::nullopt;
  }

  ssize_t startIndex = -1;
  char quote = false;

  ssize_t i = 0;
  char c;
  for (c = *input; c > 0 && i < static_cast<ssize_t>(inputStr.length());
       c = *(++input), i++) {
    if (startIndex >= 0) {
      // Closing quote or whitespace ends the token.
      if (c == quote || (!quote && std::isspace(c))) {
        quote = 0;
        break;
      }
    } else if (c == '\'' || c == '"') {
      // Enter quote - start of token.
      startIndex = i + 1;
      quote = c;
    } else if (!std::isspace(c)) {
      // Non-whitespace - start of token.
      startIndex = i;
    }
  }

  if (startIndex < 0 || quote) {
    // All whitespace or no closing quote.
    return std::nullopt;
  }

  std::string_view token = inputStr.substr(startIndex, i - startIndex);
  inputStr.remove_prefix(i + (c > 0 ? 1 : 0));
  return token;
}

bool CommandParser::ToArgs(std::string_view input,
                           std::string_view &parsedCommandName,
                           std::vector<std::string_view> &parsedArgs) {
  std::optional<std::string_view> tokenOpt = NextToken(input);
  if (!tokenOpt.has_value()) {
    return false;
  }

  parsedCommandName = tokenOpt.value();

  while (true) {
    tokenOpt = NextToken(input);
    if (!tokenOpt.has_value()) {
      break;
    }
    parsedArgs.push_back(tokenOpt.value());
  }
  return true;
}

#pragma endregion

#pragma region Command Execution

namespace {

/**
 * doCommand functions should return a json-encoded CommandResult or an empty
 * json if there is an invocation error.
 */

struct CommandResult {
  bool success = true;

  // Common problems
  bool badInvocation = false;
  bool noSuchElement = false;
};

CommandResult BadInvocationResult{.success = false, .badInvocation = true};
CommandResult NoSuchElementResult{.success = false, .noSuchElement = true};

void to_json(nlohmann::json &j, const CommandResult &result) {
  j = nlohmann::json{
      {"success", result.success},
      {"bad-invocation", result.badInvocation},
      {"no-such-element", result.noSuchElement},
  };
}

void invalidCommandInvocation(std::string_view commandName,
                              std::string_view usage,
                              std::shared_ptr<spdlog::logger> logger) {
  logger->error("invalid invocation of `{}', usage: {} {}", commandName,
                commandName, usage);
}

nlohmann::json doLoad(VirtualCanvas &vCanvas, Controller &controller,
                      std::span<std::string_view> args,
                      std::shared_ptr<spdlog::logger> logger) {
  if (args.size() != 1) {
    return BadInvocationResult;
  }
  const std::filesystem::path filePath(args[0]);

  bool success = vCanvas.loadElementConfig(filePath);

  // Controller needs to initialize events for all the new elements
  controller.reinitializeCanvasEvents();

  return CommandResult{.success = success};
}

nlohmann::json doSave(VirtualCanvas &vCanvas, Controller &controller,
                      std::span<std::string_view> args,
                      std::shared_ptr<spdlog::logger> logger) {
  if (args.size() != 1) {
    return BadInvocationResult;
  }
  const std::filesystem::path filePath(args[0]);

  vCanvas.saveElementConfig(filePath);
  return CommandResult{.success = true};
}

nlohmann::json doElementRename(VirtualCanvas &vCanvas, Controller &controller,
                               std::span<std::string_view> args,
                               std::shared_ptr<spdlog::logger> logger) {
  if (args.size() != 2) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);
  const std::string name(args[1]);

  std::shared_ptr<Element> element = vCanvas.getElement(id);
  if (!element) {
    logger->error("element-rename: no such element `{}'", id);
    return NoSuchElementResult;
  }
  element->setName(name);
  return CommandResult{.success = true};
}

nlohmann::json doElementMove(VirtualCanvas &vCanvas, Controller &controller,
                             std::span<std::string_view> args,
                             std::shared_ptr<spdlog::logger> logger) {
  if (args.size() != 3) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);
  int x, y;
  try {
    x = std::stoi(std::string(args[1]));
    y = std::stoi(std::string(args[2]));
  } catch (...) {
    return BadInvocationResult;
  }

  std::shared_ptr<Element> element = vCanvas.getElement(id);
  if (!element) {
    logger->error("element-move: no such element `{}'", id);
    return NoSuchElementResult;
  }
  element->setLocation(cv::Point(x, y));
  return CommandResult{.success = true};
}

nlohmann::json doElementRotate(VirtualCanvas &vCanvas, Controller &controller,
                               std::span<std::string_view> args,
                               std::shared_ptr<spdlog::logger> logger) {
  if (args.size() != 2) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);
  double degrees;
  try {
    degrees = std::stod(std::string(args[1]));
  } catch (...) {
    return BadInvocationResult;
  }

  std::shared_ptr<Element> element = vCanvas.getElement(id);
  if (!element) {
    logger->error("element-rotate: no such element `{}'", id);
    return NoSuchElementResult;
  }
  element->setRotation(degrees);
  return CommandResult{.success = true};
}

nlohmann::json doElementResize(VirtualCanvas &vCanvas, Controller &controller,
                               std::span<std::string_view> args,
                               std::shared_ptr<spdlog::logger> logger) {
  if (args.size() != 3) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);
  int x, y;
  try {
    x = std::stoi(std::string(args[1]));
    y = std::stoi(std::string(args[2]));
  } catch (...) {
    return BadInvocationResult;
  }

  std::shared_ptr<Element> element = vCanvas.getElement(id);
  if (!element) {
    logger->error("element-resize: no such element `{}'", id);
    return NoSuchElementResult;
  }
  element->setSize(cv::Size(x, y));
  return CommandResult{.success = true};
}

nlohmann::json doElementMoveUp(VirtualCanvas &vCanvas, Controller &controller,
                               std::span<std::string_view> args,
                               std::shared_ptr<spdlog::logger> logger) {
  if (args.size() != 1) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);

  bool success = vCanvas.moveElementUp(id);
  return CommandResult{.success = success};
}

nlohmann::json doElementMoveDown(VirtualCanvas &vCanvas, Controller &controller,
                                 std::span<std::string_view> args,
                                 std::shared_ptr<spdlog::logger> logger) {
  if (args.size() != 1) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);

  bool success = vCanvas.moveElementDown(id);
  return CommandResult{.success = success};
}

nlohmann::json doImageSetFile(VirtualCanvas &vCanvas, Controller &controller,
                              std::span<std::string_view> args,
                              std::shared_ptr<spdlog::logger> logger) {
  if (args.size() != 2) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);
  const std::string filepath(args[1]);

  auto element =
      std::dynamic_pointer_cast<ImageElement>(vCanvas.getElement(id));
  if (!element) {
    return NoSuchElementResult;
  }

  element->loadImageFile(filepath);
  return CommandResult{.success = true};
}

nlohmann::json doCarouselSetFiles(VirtualCanvas &vCanvas,
                                  Controller &controller,
                                  std::span<std::string_view> args,
                                  std::shared_ptr<spdlog::logger> logger) {
  if (args.empty()) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);

  auto element =
      std::dynamic_pointer_cast<CarouselElement>(vCanvas.getElement(id));
  if (!element) {
    return NoSuchElementResult;
  }

  if (args.size() == 1) {
    element->loadImages({});
  } else {
    std::vector<std::string> filepaths(std::next(args.begin()), args.end());
    element->loadImages(filepaths);
  }
  return CommandResult{.success = true};
}

nlohmann::json doCarouselSetFrameRate(VirtualCanvas &vCanvas,
                                      Controller &controller,
                                      std::span<std::string_view> args,
                                      std::shared_ptr<spdlog::logger> logger) {
  if (args.size() < 2) {
    return BadInvocationResult;
  }
  const std::string id(args[0]);
  int frameRate;
  try {
    frameRate = std::stoi(std::string(args[1]));
  } catch (...) {
    return BadInvocationResult;
  }

  auto element =
      std::dynamic_pointer_cast<CarouselElement>(vCanvas.getElement(id));
  if (!element) {
    return NoSuchElementResult;
  }

  element->setFrameRate(frameRate);

  // The element's event period was changed.
  controller.reinitializeCanvasEvents();

  return CommandResult{.success = true};
}

using DoCommandFunc = nlohmann::json(VirtualCanvas &vCanvas,
                                     Controller &controller,
                                     std::span<std::string_view> args,
                                     std::shared_ptr<spdlog::logger> logger);

struct CommandInfo {
  DoCommandFunc *doCommandFunc;
  std::string usage;
};

std::map<std::string, CommandInfo> commands{
    {
        "load",
        CommandInfo{
            .doCommandFunc = doLoad,
            .usage = "<filepath>",
        },
    },
    {
        "save",
        CommandInfo{
            .doCommandFunc = doSave,
            .usage = "<filepath>",
        },
    },
    {
        "element-rename",
        CommandInfo{
            .doCommandFunc = doElementRename,
            .usage = "<id> <name>",
        },
    },
    {
        "element-move",
        CommandInfo{
            .doCommandFunc = doElementMove,
            .usage = "<id> <x> <y>",
        },
    },
    {
        "element-rotate",
        CommandInfo{
            .doCommandFunc = doElementRotate,
            .usage = "<id> <degrees>",
        },
    },
    {
        "element-resize",
        CommandInfo{
            .doCommandFunc = doElementResize,
            .usage = "<id> <width> <height>",
        },
    },
    {
        "element-move-up",
        CommandInfo{
            .doCommandFunc = doElementMoveUp,
            .usage = "<id>",
        },
    },
    {
        "element-move-down",
        CommandInfo{
            .doCommandFunc = doElementMoveDown,
            .usage = "<id>",
        },
    },
    {
        "image-set-file",
        CommandInfo{
            .doCommandFunc = doImageSetFile,
            .usage = "<id> <filepath>",
        },
    },
    {
        "carousel-set-files",
        CommandInfo{
            .doCommandFunc = doCarouselSetFiles,
            .usage = "<id> [filepaths...]",
        },
    },
    {
        "carousel-set-framerate",
        CommandInfo{
            .doCommandFunc = doCarouselSetFrameRate,
            .usage = "<id> <framerate>",
        },
    },
};

void doHelp(std::shared_ptr<spdlog::logger> logger) {
  std::stringstream ss;

  ss << "usage:\nAvailable commands:\n";
  ss << "\thelp\n\tquit\n\tpause\n\tresume";

  for (const auto &[name, info] : commands) {
    ss << "\n\t" << name << " " << info.usage;
  }

  logger->info(ss.str());
}

} // namespace

nlohmann::json ProcessCommand(VirtualCanvas &vCanvas, Controller &controller,
                              const std::string &line, bool &isPaused,
                              bool &isRunning,
                              std::shared_ptr<spdlog::logger> logger) {
  spdlog::info("[Command] ProcessCommand: `{}'", line);

  std::string_view commandName;
  std::vector<std::string_view> args;
  if (!CommandParser::ToArgs(line, commandName, args)) {
    logger->error("failed to parse arguments: `{}'", line);
    return BadInvocationResult;
  }

  // TODO: Commands to edit each element and get canvas data.

  if (commandName == "help") {
    doHelp(logger);
  } else if (commandName == "quit" || commandName == "exit") {
    isRunning = false;
  } else if (commandName == "pause") {
    isPaused = true;
  } else if (commandName == "resume") {
    isPaused = false;
  } else if (const auto it = commands.find(std::string(commandName));
             it != commands.end()) {
    auto &[doCommandFunc, usage] = it->second;
    nlohmann::json result = doCommandFunc(vCanvas, controller, args, logger);
    if (result["bad-invocation"].get<bool>()) {
      invalidCommandInvocation(commandName, usage, logger);
    }
    return result;
  } else {
    logger->error("no such command `{}'", commandName);
    return BadInvocationResult;
  }

  return CommandResult{.success = true};
}

#pragma endregion

#pragma region Prompt Command Source

PromptCommandSource::PromptCommandSource() {
  m_cmdOutputLogger = spdlog::default_logger()->clone("LEDVW-Command-Prompt");
  m_cmdOutputLogger->sinks().push_back(
      std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

  rl_catch_signals = false;

  rl_event_hook = []() -> int {
    if (!get().m_isRunning) {
      rl_done = true;
    }
    return 0;
  };

  m_thread = std::jthread(&PromptCommandSource::threadFunc, this);
}

PromptCommandSource::~PromptCommandSource() { m_isRunning = false; }

void PromptCommandSource::setPromptName(std::string_view name) {
  std::lock_guard lock(m_mutex);
  m_promptName = name;
}

std::string PromptCommandSource::consumeLatestCommand() {
  std::lock_guard lock(m_mutex);
  if (m_cmdQueue.empty()) {
    return "";
  }
  std::string cmd = m_cmdQueue.front();
  m_cmdQueue.pop();
  return cmd;
}

void PromptCommandSource::threadFunc() {
  while (m_isRunning) {
    if (!m_isActive) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      continue;
    }

    std::string prompt;
    {
      std::lock_guard lock(m_mutex);
      prompt = m_promptName;
    }
    prompt += "> ";

    char *line = readline(prompt.c_str());
    if (!line || !m_isRunning) {
      break;
    }

    if (*line != '\0') {
      add_history(line);
    }

    std::string lineString(line);
    free(line);

    {
      std::lock_guard lock(m_mutex);
      m_cmdQueue.push(lineString);
      m_isActive = false;
    }

    if (lineString == "exit" || lineString == "quit") {
      //  break;
    }
  }
}

#pragma endregion

#pragma region Unix Socket Command Source

UnixSocketCommandSource::UnixSocketCommandSource(
    const std::filesystem::path &path)
    : m_channel(path) {
  m_cmdOutputLogger = spdlog::default_logger()->clone("LEDVW-Command");

  m_lastHeartbeatTime = std::chrono::steady_clock::now();
}

void UnixSocketCommandSource::process() {
  m_channel.process();

  auto now = std::chrono::steady_clock::now();
  if (m_channel.isConnected()) {
    auto [success, receiveBuffer] = m_channel.receiveMessage();
    if (success && !receiveBuffer.empty()) {
      std::string commandString(
          reinterpret_cast<const char *>(receiveBuffer.data()),
          receiveBuffer.size());
      m_cmdQueue.push(commandString);
    }

    // Send heartbeat messages every second to verify that we're still
    // connected.
    auto elapsed = now - m_lastHeartbeatTime;
    if (elapsed > std::chrono::milliseconds(1000)) {
      m_lastHeartbeatTime = now;
      m_channel.sendMessage({});
    }
  } else {
    m_lastHeartbeatTime = now;
  }
}

std::string UnixSocketCommandSource::consumeLatestCommand() {
  if (m_cmdQueue.empty()) {
    return "";
  }

  std::string cmd = m_cmdQueue.front();
  m_cmdQueue.pop();
  return cmd;
}

void UnixSocketCommandSource::handleResponse(const nlohmann::json &response) {
  std::string responseString = response.dump();
  std::span responseBuffer(
      reinterpret_cast<const uint8_t *>(responseString.data()),
      responseString.size());
  m_channel.sendMessage(responseBuffer);
}

#pragma endregion
