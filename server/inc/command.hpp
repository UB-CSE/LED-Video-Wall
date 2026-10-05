#pragma once

#include "canvas.hpp"
#include "controller.hpp"
#include "unix-socket-msg-channel-tests.hpp"
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
 * @param controller Controller.
 * @param line Command line string to parse.
 * @param isPaused (Output) Will be assigned if the paused state gets changed by
 *                 pause & resume commands.
 * @param isRunning (Output) Will be assigned to `true` if a quit command is
 *                  run.
 * @param logger The command output logger.
 * @return JSON Result, or an empty JSON if there was an invocation error.
 */
nlohmann::json ProcessCommand(VirtualCanvas &vCanvas, Controller &controller,
                              const std::string &line, bool &isPaused,
                              bool &isRunning,
                              std::shared_ptr<spdlog::logger> logger);

class CommandSource {
public:
  virtual ~CommandSource() = default;

  virtual void process() {}

  virtual std::string consumeLatestCommand() = 0;
  virtual void handleResponse(const nlohmann::json &response) {};

  virtual std::shared_ptr<spdlog::logger> getCommandOutputLogger() {
    return spdlog::default_logger();
  }

protected:
  CommandSource() = default;
};

/**
 * A GNU readline command prompt. Processes in a separate thread. Starts up at
 * the first access.
 */
class PromptCommandSource : public CommandSource {
  std::jthread m_thread;

  std::mutex m_mutex;
  std::queue<std::string> m_cmdQueue;

  bool m_isRunning = true;
  bool m_isActive = false;

  std::string m_promptName = "ledvw";

  std::shared_ptr<spdlog::logger> m_cmdOutputLogger;

public:
  static PromptCommandSource &get() {
    static PromptCommandSource instance;
    return instance;
  }

  ~PromptCommandSource() override;

  PromptCommandSource(const PromptCommandSource &) = delete;
  PromptCommandSource &operator=(const PromptCommandSource &) = delete;

  void setPromptName(std::string_view name);

  void activate() { m_isActive = true; }
  std::string consumeLatestCommand() override;

  std::shared_ptr<spdlog::logger> getCommandOutputLogger() override {
    return m_cmdOutputLogger;
  }

private:
  PromptCommandSource();

  void threadFunc();
};

class UnixSocketCommandSource : public CommandSource {
  UnixSocketMessageChannel<MessageChannel::Server> m_channel;

  std::queue<std::string> m_cmdQueue;

  std::chrono::steady_clock::time_point m_lastHeartbeatTime;

  std::shared_ptr<spdlog::logger> m_cmdOutputLogger;

public:
  UnixSocketCommandSource(const std::filesystem::path &path);
  ~UnixSocketCommandSource() override = default;

  void process() override;

  std::string consumeLatestCommand() override;
  void handleResponse(const nlohmann::json &response) override;

  std::shared_ptr<spdlog::logger> getCommandOutputLogger() override {
    return m_cmdOutputLogger;
  }
};
