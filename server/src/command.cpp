#include "command.hpp"
#include "canvas.hpp"
#include "text-render.hpp"
#include <cctype>
#include <iostream>
#include <optional>
#include <string>
#include <sys/types.h>

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

nlohmann::json ProcessCommand(VirtualCanvas &vCanvas, const std::string &line,
                              bool &isPaused, bool &isRunning) {
  std::string_view commandName;
  std::vector<std::string_view> args;
  if (!CommandParser::ToArgs(line, commandName, args)) {
    std::cerr << "Invalid command invocation\n";
    return {};
  }

  // TODO: Implement Commands

  return {};
}
