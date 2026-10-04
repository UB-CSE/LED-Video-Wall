#pragma once

#include "canvas.hpp"
#include <nlohmann/json.hpp>
#include <optional>
#include <string_view>

struct CommandParser {
  /**
   * Get the next token from a string.
   *
   * Tokens are separated by whitespace.
   *
   * Text between two single quotes or between two double quotes is treated as a
   * token.
   *
   * @param input Input string. When successful, this gets iterated past the
   *              acquired token.
   * @return The token string view if successful, or std::nullopt if no token
   *         could be acquired.
   */
  static std::optional<std::string_view> NextToken(std::string_view &input);

  /**
   * Parses a command line string into a command and arguments.
   *
   * @param input Command line string.
   * @param parsedCommandName First token in the string (command name).
   * @param parsedArgs The rest of the tokens (the command's arguments).
   * @return Success
   */
  static bool ToArgs(std::string_view input,
                     std::string_view &parsedCommandName,
                     std::vector<std::string_view> &parsedArgs);
};

/**
 * Parses and executes a command.
 *
 * @param vCanvas Canvas.
 * @param line Command line string to parse.
 * @param isPaused (Output) Will be assigned if the paused state gets changed by
 *                 pause & resume commands.
 * @param isRunning (Output) Will be assigned to `true` if a quit command is
 *                  run.
 * @return JSON Result, or an empty JSON if there was an invocation error.
 */
nlohmann::json ProcessCommand(VirtualCanvas &vCanvas, const std::string &line,
                              bool &isPaused, bool &isRunning);
