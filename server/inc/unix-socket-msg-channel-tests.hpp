// Adapted from https://codeberg.org/petelilley/micromouse/src/branch/main/libs/base/include/mm/messaging/socket-message-channel.hpp

#pragma once

#include "msg-channel.hpp"

#include <filesystem>
#include <span>
#include <cstdint>
#include <sys/types.h>

template <MessageChannel::Side Side> class UnixSocketMessageChannel;

class UnixSocketMessageChannelCommon {
protected:
  using LengthType = uint32_t;

  static bool setSocketNonBlocking(int socketFd, bool nonBlocking);

  /**
   * Reads a message length, then message data. Returns true if the message was
   * partially/completely read and false if an error was encountered.
   *
   * @param socketFd
   * @return True on success, false on connection error.
   */
  bool readMessage(int socketFd);

  static bool writeMessage(int socketFd, std::span<const uint8_t> message);

  void releaseMessage();

  uint8_t m_receiveBuffer[1024] = {};
  ssize_t m_numBytesReceived = 0;

  bool m_isHoldingMessage = false;
  ssize_t m_expectedMessageLength = -1;
};

// Server side of a message channel communicating over a UNIX socket.
template <>
class UnixSocketMessageChannel<MessageChannel::Server> final
    : public MessageChannel,
      private UnixSocketMessageChannelCommon {
public:
  UnixSocketMessageChannel(const std::filesystem::path &socketPath);
  ~UnixSocketMessageChannel() override;

  void process() override;

  bool sendMessage(std::span<const uint8_t> message) override;
  ReceivedMessage receiveMessage() override;

  bool isConnected() const override { return m_clientFd > 0; }

  void setConnectionCallback(ConnectionCallback callback) override {
    m_connectionCallback = std::move(callback);
  }

private:
  void initializeSocket();
  void cleanupSocket();

  bool connectToClient();
  void closeClientConnection();

private:
  ConnectionCallback m_connectionCallback;

  std::filesystem::path m_socketPath;
  int m_socketFd = -1;
  bool m_isInitialized = false;

  int m_clientFd = -1;
};

// Client side of a message channel communicating over a UNIX socket.
template <>
class UnixSocketMessageChannel<MessageChannel::Client>
    : public MessageChannel, private UnixSocketMessageChannelCommon {
public:
  UnixSocketMessageChannel(const std::filesystem::path &socketPath);
  ~UnixSocketMessageChannel() override;

  void process() override;

  bool sendMessage(std::span<const uint8_t> message) override;
  ReceivedMessage receiveMessage() override;

  bool isConnected() const override { return m_socketFd > 0; }

  void setConnectionCallback(ConnectionCallback callback) override {
    m_connectionCallback = std::move(callback);
  }

private:
  bool initializeSocket();
  void cleanupSocket();

  void closeConnection();

private:
  ConnectionCallback m_connectionCallback;

  std::filesystem::path m_socketPath;
  int m_socketFd = -1;
};
