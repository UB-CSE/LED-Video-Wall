// Adapted from https://codeberg.org/petelilley/micromouse/src/branch/main/libs/base/lib/messaging/socket-message-channel.cpp

#include "unix-socket-msg-channel-tests.hpp"

#include <cerrno>
#include <csignal>
#include <cstring>
#include <spdlog/spdlog.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

using enum MessageChannel::Side;

static bool isNoDataAvailable() {
  return (errno == EWOULDBLOCK || errno == EAGAIN);
}

#pragma region UnixSocketMessageChannelCommon

UnixSocketMessageChannelCommon::~UnixSocketMessageChannelCommon() {
  if (m_messageContent) {
    delete[] m_messageContent;
  }
}

bool UnixSocketMessageChannelCommon::setSocketNonBlocking(int socketFd,
                                                          bool nonBlocking) {
  if (socketFd < 0) {
    return false;
  }

  int flags = fcntl(socketFd, F_GETFL, 0);
  if (flags < 0) {
    spdlog::error("[UnixSocket] failed to get socket flags: {}",
                  std::strerror(errno));
    return false;
  }

  if (nonBlocking) {
    flags |= O_NONBLOCK;
  } else {
    flags &= ~O_NONBLOCK;
  }

  if (fcntl(socketFd, F_SETFL, flags) < 0) {
    spdlog::error("[UnixSocket] failed to set socket non-blocking flag: {}",
                  std::strerror(errno));
    return false;
  }

  return true;
}

bool UnixSocketMessageChannelCommon::readMessage(int socketFd) {
  if (m_isHoldingMessage) {
    return true;
  }

  // Read message length.

  if (m_expectedMessageLength < 0) {
    uint8_t buffer[sizeof(LengthType)];

    if (m_numBytesReceived < static_cast<ssize_t>(sizeof(LengthType))) {

      const size_t numBytesLeft = sizeof(LengthType) - m_numBytesReceived;

      const ssize_t numBytesRead =
          read(socketFd, buffer + m_numBytesReceived, numBytesLeft);
      if (numBytesRead < 0) {
        if (isNoDataAvailable()) {
          return true;
        }

        spdlog::error("[UnixSocket] failed to read: {}", std::strerror(errno));
        return false;
      }

      m_numBytesReceived += numBytesRead;
    }

    if (m_numBytesReceived < static_cast<ssize_t>(sizeof(LengthType))) {
      return true;
    }

    m_expectedMessageLength = *reinterpret_cast<LengthType *>(buffer);
    if (m_messageContent) {
      delete[] m_messageContent;
    }
    m_messageContent = new uint8_t[m_expectedMessageLength];
    m_numBytesReceived = 0;
  }

  // Read message data.

  if (m_numBytesReceived < m_expectedMessageLength) {
    const size_t numBytesLeft = m_expectedMessageLength - m_numBytesReceived;

    const ssize_t numBytesRead =
        read(socketFd, m_messageContent + m_numBytesReceived, numBytesLeft);
    if (numBytesRead < 0) {
      if (isNoDataAvailable()) {
        return true;
      }

      spdlog::error("[UnixSocket] failed to read: {}", std::strerror(errno));
      return false;
    }

    m_numBytesReceived += numBytesRead;
  }

  if (m_numBytesReceived < m_expectedMessageLength) {
    return true;
  }

  m_isHoldingMessage = true;

  return true;
}

bool UnixSocketMessageChannelCommon::writeMessage(
    int socketFd, std::span<const uint8_t> message) {
  // Message length

  const auto messageLength = static_cast<LengthType>(message.size());

  ssize_t numBytesWritten = write(socketFd, &messageLength, sizeof(LengthType));
  if (numBytesWritten != sizeof(LengthType)) {
    spdlog::error("[UnixSocket] failed to write: {}", std::strerror(errno));
    return false;
  }

  // Message data

  numBytesWritten = write(socketFd, message.data(), messageLength);
  if (numBytesWritten != messageLength) {
    spdlog::error("[UnixSocket] failed to write: {}", std::strerror(errno));
    return false;
  }

  return true;
}

void UnixSocketMessageChannelCommon::releaseMessage() {
  m_isHoldingMessage = false;
  m_numBytesReceived = 0;
  m_expectedMessageLength = -1;
}

#pragma endregion

#pragma region UnixSocketMessageChannel<Server>

UnixSocketMessageChannel<Server>::UnixSocketMessageChannel(
    const std::filesystem::path &socketPath)
    : m_socketPath(socketPath) {
  std::signal(SIGPIPE, SIG_IGN);
  initializeSocket();
}

UnixSocketMessageChannel<Server>::~UnixSocketMessageChannel() {
  cleanupSocket();
}

void UnixSocketMessageChannel<Server>::process() {
  if (!m_isInitialized) {
    return;
  }

  if (!isConnected()) {
    if (!connectToClient()) {
      return;
    }
  }

  if (!readMessage(m_clientFd)) {
    closeClientConnection();
  }
}

bool UnixSocketMessageChannel<Server>::sendMessage(
    std::span<const uint8_t> message) {
  if (!isConnected()) {
    return false;
  }

  // The entire message needs to get written.
  (void)setSocketNonBlocking(m_clientFd, false);

  const bool result = writeMessage(m_clientFd, message);
  if (!result) {
    closeClientConnection();
  }

  (void)setSocketNonBlocking(m_clientFd, true);

  return result;
}

MessageChannel::ReceivedMessage
UnixSocketMessageChannel<Server>::receiveMessage() {
  if (!isConnected()) {
    return ReceivedMessage{.success = false, .message = {}};
  }

  if (!m_isHoldingMessage) {
    return ReceivedMessage{.success = true, .message = {}};
  }

  std::span<const uint8_t> result(m_messageContent,
                                  static_cast<size_t>(m_expectedMessageLength));
  releaseMessage();

  return ReceivedMessage{.success = true, .message = result};
}

void UnixSocketMessageChannel<Server>::initializeSocket() {
  struct sockaddr_un addr{};

  m_socketFd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (m_socketFd < 0) {
    spdlog::error("[UnixSocket] failed to create socket: {}",
                  std::strerror(errno));
    return;
  }

  std::string socketPathString = m_socketPath.string();

  if (std::filesystem::exists(m_socketPath)) {
    spdlog::warn(
        "[UnixSocket] socket file `{}' already exists, attempting to remove it",
        m_socketPath.string());
    std::error_code ec;
    if (!std::filesystem::remove(m_socketPath, ec)) {
      spdlog::error("[UnixSocket] failed to remove socket file `{}': {}",
                    m_socketPath.string(), ec.message());
      goto ERROR;
    }
  }

  std::memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  std::strncpy(addr.sun_path, socketPathString.data(),
               std::min(sizeof(addr.sun_path) - 1, socketPathString.size()));

  if (bind(m_socketFd, reinterpret_cast<const struct sockaddr *>(&addr),
           sizeof(struct sockaddr_un)) < 0) {
    spdlog::error("[UnixSocket] failed to bind socket: {}",
                  std::strerror(errno));
    goto ERROR;
  }

  if (listen(m_socketFd, 1) < 0) {
    spdlog::error("[UnixSocket] failed to listen socket: {}",
                  std::strerror(errno));
    goto ERROR;
  }

  if (!setSocketNonBlocking(m_socketFd, true)) {
    goto ERROR;
  }

  m_isInitialized = true;
  return;

ERROR:
  cleanupSocket();
}

void UnixSocketMessageChannel<Server>::cleanupSocket() {
  if (m_clientFd > 0) {
    close(m_clientFd);
    m_clientFd = -1;
  }
  if (m_socketFd > 0) {
    close(m_socketFd);
    m_socketFd = -1;
  }
  releaseMessage();

  if (std::filesystem::exists(m_socketPath)) {
    std::error_code ec;
    if (!std::filesystem::remove(m_socketPath, ec)) {
      spdlog::error("[UnixSocket] failed to remove socket file `{}': {}",
                    m_socketPath.string(), ec.message());
    }
  }
}

bool UnixSocketMessageChannel<Server>::connectToClient() {
  m_clientFd = accept(m_socketFd, nullptr, nullptr);
  if (m_clientFd < 0) {
    if (errno == EWOULDBLOCK || errno == EAGAIN) {
      // Nothing to accept.
    } else {
      spdlog::error("[UnixSocket] failed to accept client connection: {}",
                    std::strerror(errno));
    }
    return false;
  }

  if (!setSocketNonBlocking(m_clientFd, true)) {
    closeClientConnection();
    return false;
  }

  spdlog::info("[UnixSocket] client connected");

  if (m_connectionCallback) {
    m_connectionCallback(true);
  }

  return true;
}

void UnixSocketMessageChannel<Server>::closeClientConnection() {
  if (m_clientFd > 0) {
    close(m_clientFd);
    m_clientFd = -1;
    if (m_connectionCallback) {
      m_connectionCallback(false);
    }
  }
  if (!m_isHoldingMessage) {
    releaseMessage();
  }

  spdlog::info("[UnixSocket] client disconnected");
}

#pragma endregion

#pragma region UnixSocketMessageChannel<Client>

UnixSocketMessageChannel<Client>::UnixSocketMessageChannel(
    const std::filesystem::path &socketPath)
    : m_socketPath(socketPath) {
  std::signal(SIGPIPE, SIG_IGN);
  initializeSocket();
}

UnixSocketMessageChannel<Client>::~UnixSocketMessageChannel() {
  cleanupSocket();
}

void UnixSocketMessageChannel<Client>::process() {
  if (!isConnected()) {
    if (!initializeSocket()) {
      return;
    }
  }

  if (!std::filesystem::exists(m_socketPath)) {
    closeConnection();
    return;
  }

  if (!readMessage(m_socketFd)) {
    closeConnection();
    return;
  }
}

bool UnixSocketMessageChannel<Client>::sendMessage(
    std::span<const uint8_t> message) {
  if (!isConnected()) {
    return false;
  }

  // The entire message needs to get written.
  (void)setSocketNonBlocking(m_socketFd, false);

  const bool result = writeMessage(m_socketFd, message);
  if (!result) {
    closeConnection();
  }

  (void)setSocketNonBlocking(m_socketFd, true);

  return result;
}

MessageChannel::ReceivedMessage
UnixSocketMessageChannel<Client>::receiveMessage() {
  if (!isConnected()) {
    return ReceivedMessage{.success = false, .message = {}};
  }

  if (!m_isHoldingMessage) {
    return ReceivedMessage{.success = true, .message = {}};
  }

  std::span<const uint8_t> result(m_messageContent,
                                  static_cast<size_t>(m_expectedMessageLength));
  releaseMessage();

  return ReceivedMessage{.success = true, .message = result};
}

bool UnixSocketMessageChannel<Client>::initializeSocket() {
  if (!std::filesystem::exists(m_socketPath)) {
    return false;
  }
  const std::string socketPathString = m_socketPath.string();

  m_socketFd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (m_socketFd < 0) {
    spdlog::error("[UnixSocket] failed to create socket: {}",
                  std::strerror(errno));
    return false;
  }

  if (!setSocketNonBlocking(m_socketFd, true)) {
    goto ERROR;
  }

  {
    struct sockaddr_un addr = {};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socketPathString.data(),
                 std::min(sizeof(addr.sun_path) - 1, socketPathString.size()));

    if (connect(m_socketFd, reinterpret_cast<const struct sockaddr *>(&addr),
                sizeof(struct sockaddr_un)) < 0) {
      spdlog::error("[UnixSocket] failed to connect to server socket: {}",
                    std::strerror(errno));
      goto ERROR;
    }
  }

  spdlog::info("[UnixSocket] server connected at `{}'", m_socketPath.string());

  if (m_connectionCallback) {
    m_connectionCallback(true);
  }

  return true;

ERROR:
  cleanupSocket();
  return false;
}

void UnixSocketMessageChannel<Client>::cleanupSocket() {
  if (m_socketFd > 0) {
    close(m_socketFd);
    m_socketFd = -1;
  }
  releaseMessage();
}

void UnixSocketMessageChannel<Client>::closeConnection() {
  cleanupSocket();
  if (m_connectionCallback) {
    m_connectionCallback(false);
  }

  spdlog::info("[UnixSocket] server disconnected");
}

#pragma endregion
