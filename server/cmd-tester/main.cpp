/**
 * A simple command-line utility program to connect to the LEDVW server and send
 * a command to it.
 */

#include "unix-socket-msg-channel-tests.hpp"
#include <filesystem>
#include <iostream>
#include <spdlog/spdlog.h>
#include <string>

using namespace std::string_literals;

static constexpr auto ResponseWaitTime = std::chrono::milliseconds(1000);

int main(int argc, const char **argv) {
  if (argc < 3) {
    std::cerr << "usage: " << argv[0]
              << " <socket-file> <command-name> [args]\n";
    return 1;
  }

  const std::filesystem::path socketFile(argv[1]);
  if (!std::filesystem::exists(socketFile)) {
    spdlog::error("no such socket file `{}'", socketFile.string());
    return 1;
  }

  std::string commandString = argv[2];
  for (int i = 3; i < argc; i++) {
    commandString += " "s + argv[i];
  }

  UnixSocketMessageChannel<MessageChannel::Client> msgChannel(socketFile);
  if (!msgChannel.isConnected()) {
    return 1;
  }

  std::span commandStringBuffer(
      reinterpret_cast<const uint8_t *>(commandString.data()),
      commandString.size());

  if (!msgChannel.sendMessage(commandStringBuffer)) {
    return 1;
  }

  std::string response;

  auto start = std::chrono::steady_clock::now();
  while (true) {
    auto elapsed = std::chrono::steady_clock::now() - start;
    if (elapsed > ResponseWaitTime) {
      spdlog::error("response timeout");
      return 1;
    }

    msgChannel.process();

    auto [success, responseBuffer] = msgChannel.receiveMessage();
    if (!success) {
      return 1;
    }

    if (!responseBuffer.empty()) {
      response = std::string(responseBuffer.begin(), responseBuffer.end());
      break;
    }
  }

  spdlog::info("received response!\n\n{}\n", response);

  return 0;
}