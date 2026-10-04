#pragma once

#include "canvas.hpp"
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <queue>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <thread>

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
 * @param logger The command output logger.
 * @return JSON Result, or an empty JSON if there was an invocation error.
 */
nlohmann::json ProcessCommand(VirtualCanvas &vCanvas, const std::string &line,
                              bool &isPaused, bool &isRunning,
                              std::shared_ptr<spdlog::logger> logger);

/**
 * A GNU readline command prompt. Processes in a separate thread. Starts up at
 * the first access.
 */
class InteractiveCommandPrompt {
  std::jthread m_thread;

  std::mutex m_mutex;
  std::queue<std::string> m_cmdQueue;

  bool m_isRunning = true;
  bool m_isActive = true;

  std::string m_promptName = "ledvw";

public:
  static InteractiveCommandPrompt &get() {
    static InteractiveCommandPrompt instance;
    return instance;
  }

  ~InteractiveCommandPrompt();

  InteractiveCommandPrompt(const InteractiveCommandPrompt &) = delete;
  InteractiveCommandPrompt &
  operator=(const InteractiveCommandPrompt &) = delete;

  void setPromptName(std::string_view name);

  void activate() { m_isActive = true; }
  std::string consumeLatestCommand();

private:
  InteractiveCommandPrompt();

  void threadFunc();
};
