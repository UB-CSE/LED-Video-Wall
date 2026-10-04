#include "command.hpp"
#include "canvas.hpp"
#include "text-render.hpp"
#include <cctype>
#include <iostream>
#include <optional>
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

namespace {

/**
 * doCommand functions should return a json-encoded CommandResult or an empty
 * json if there is an invocation error.
 */

struct CommandResult {
  bool success = true;
};

void to_json(nlohmann::json &j, const CommandResult &result) {
  j = nlohmann::json{
      {"success", result.success},
  };
}

void doHelp() {}

nlohmann::json doElementRename(VirtualCanvas &vCanvas,
                               std::span<std::string_view> args) {
  auto usage = [] { puts("usage: element-rename <id> <name>"); };

  if (args.size() != 2) {
    usage();
    return {};
  }
  const std::string id(args[0]);
  const std::string name(args[1]);

  std::shared_ptr<Element> element = vCanvas.getElement(id);
  if (!element) {
    printf("error: no element exists with id %s\n", id.c_str());
    return {};
  }
  element->setName(name);
  return CommandResult{.success = true};
}

nlohmann::json doElementMove(VirtualCanvas &vCanvas,
                             std::span<std::string_view> args) {
  auto usage = [] { puts("usage: element-move <id> <x> <y>"); };

  if (args.size() != 3) {
    usage();
    return {};
  }
  const std::string id(args[0]);
  int x, y;
  try {
    x = std::stoi(std::string(args[1]));
    y = std::stoi(std::string(args[2]));
  } catch (...) {
    usage();
    return {};
  }

  std::shared_ptr<Element> element = vCanvas.getElement(id);
  if (!element) {
    printf("error: no element exists with id %s\n", id.c_str());
    return {};
  }
  element->setLocation(cv::Point(x, y));
  return CommandResult{.success = true};
}

nlohmann::json doElementRotate(VirtualCanvas &vCanvas,
                               std::span<std::string_view> args) {
  auto usage = [] { puts("usage: element-rotate <id> <degrees>"); };

  if (args.size() != 2) {
    usage();
    return {};
  }
  const std::string id(args[0]);
  double degrees;
  try {
    degrees = std::stod(std::string(args[1]));
  } catch (...) {
    usage();
    return {};
  }

  std::shared_ptr<Element> element = vCanvas.getElement(id);
  if (!element) {
    printf("error: no element exists with id %s\n", id.c_str());
    return {};
  }
  element->setRotation(degrees);
  return CommandResult{.success = true};
}

nlohmann::json doElementResize(VirtualCanvas &vCanvas,
                               std::span<std::string_view> args) {
  auto usage = [] { puts("usage: element-resize <id> <x> <y>"); };

  if (args.size() != 3) {
    usage();
    return {};
  }
  const std::string id(args[0]);
  int x, y;
  try {
    x = std::stoi(std::string(args[1]));
    y = std::stoi(std::string(args[2]));
  } catch (...) {
    usage();
    return {};
  }

  std::shared_ptr<Element> element = vCanvas.getElement(id);
  if (!element) {
    printf("error: no element exists with id %s\n", id.c_str());
    return {};
  }
  element->setSize(cv::Size(x, y));
  return CommandResult{.success = true};
}

nlohmann::json doElementMoveUp(VirtualCanvas &vCanvas,
                               std::span<std::string_view> args) {
  auto usage = [] { puts("usage: element-move-up <id>"); };

  if (args.size() != 1) {
    usage();
    return {};
  }
  const std::string id(args[0]);

  bool success = vCanvas.moveElementUp(id);
  return CommandResult{.success = success};
}

nlohmann::json doElementMoveDown(VirtualCanvas &vCanvas,
                                 std::span<std::string_view> args) {
  auto usage = [] { puts("usage: element-move-down <id>"); };

  if (args.size() != 1) {
    usage();
    return {};
  }
  const std::string id(args[0]);

  bool success = vCanvas.moveElementDown(id);
  return CommandResult{.success = success};
}

} // namespace

nlohmann::json ProcessCommand(VirtualCanvas &vCanvas, const std::string &line,
                              bool &isPaused, bool &isRunning) {
  std::string_view commandName;
  std::vector<std::string_view> args;
  if (!CommandParser::ToArgs(line, commandName, args)) {
    std::cerr << "Invalid command invocation\n";
    return {};
  }

  // TODO: Commands to edit each element and get canvas data.

  if (commandName == "help") {
    doHelp();
  } else if (commandName == "quit") {
    isRunning = false;
  } else if (commandName == "pause") {
    isPaused = true;
  } else if (commandName == "resume") {
    isPaused = false;
  } else if (commandName == "element-rename") {
    return doElementRename(vCanvas, args);
  } else if (commandName == "element-move") {
    return doElementMove(vCanvas, args);
  } else if (commandName == "element-rotate") {
    return doElementRotate(vCanvas, args);
  } else if (commandName == "element-resize") {
    return doElementResize(vCanvas, args);
  } else if (commandName == "element-move-up") {
    return doElementMoveUp(vCanvas, args);
  } else if (commandName == "element-move-down") {
    return doElementMoveDown(vCanvas, args);
  } else {
    std::cerr << "Command not found: '" << commandName << "'\n";
    return {};
  }

  return CommandResult{.success = true};
}
