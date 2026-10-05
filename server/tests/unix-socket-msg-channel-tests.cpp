// Adapted from
// https://codeberg.org/petelilley/micromouse/src/branch/main/libs/base/tests/messaging/test-socket-message-channel.cpp

#include <tests-util.hpp>

#include "msg-channel.hpp"
#include "unix-socket-msg-channel-tests.hpp"
#include <chrono>
#include <filesystem>

using namespace testing;
using enum MessageChannel::Side;

namespace {

const std::filesystem::path TestingSocketPath = "/tmp/testing_socket";

constexpr auto ConnectionTimeout = std::chrono::milliseconds(100);
constexpr auto MessageReceiveTimeout = std::chrono::milliseconds(100);

class UnixSocketMsgChannel : public Test {
protected:
  UnixSocketMsgChannel() {
    initializeServerChannel();
    initializeClientChannel();
  }

  ~UnixSocketMsgChannel() override {
    clientChannel.reset();
    serverChannel.reset();
  }

  void initializeServerChannel() {
    serverChannel =
        std::make_shared<UnixSocketMessageChannel<Server>>(TestingSocketPath);
  }

  void initializeClientChannel() {
    clientChannel =
        std::make_shared<UnixSocketMessageChannel<Client>>(TestingSocketPath);
  }

  bool waitForConnection() const {
    auto start = std::chrono::steady_clock::now();
    while ((std::chrono::steady_clock::now() - start) < ConnectionTimeout) {
      if (serverChannel->isConnected() && clientChannel->isConnected()) {
        return true;
      }
      spin();
    }
    return false;
  }

  MessageChannel::ReceivedMessage
  receiveMessage(const std::shared_ptr<MessageChannel> &channel) const {
    MessageChannel::ReceivedMessage result{.success = true, .message = {}};

    auto start = std::chrono::steady_clock::now();
    while ((std::chrono::steady_clock::now() - start) < MessageReceiveTimeout &&
           result.message.empty()) {
      spin();
      result = channel->receiveMessage();
    }

    return result;
  }

  void spin() const {
    if (serverChannel) {
      serverChannel->process();
    }
    if (clientChannel) {
      clientChannel->process();
    }
  }

  std::shared_ptr<UnixSocketMessageChannel<Server>> serverChannel;
  std::shared_ptr<UnixSocketMessageChannel<Client>> clientChannel;
};

} // namespace

TEST_F(UnixSocketMsgChannel, Basic) {
  ASSERT_THAT(std::filesystem::exists(TestingSocketPath), IsTrue());

  ASSERT_THAT(waitForConnection(), IsTrue());

  ASSERT_THAT(serverChannel->isConnected(), IsTrue());
  ASSERT_THAT(clientChannel->isConnected(), IsTrue());

  spin();

  ASSERT_THAT(serverChannel->receiveMessage().success, IsTrue());
  ASSERT_THAT(serverChannel->receiveMessage().message, IsEmpty());
  ASSERT_THAT(clientChannel->receiveMessage().success, IsTrue());
  ASSERT_THAT(clientChannel->receiveMessage().message, IsEmpty());

  std::vector<uint8_t> message;

  // Send messages

  message = {0x1, 0x2, 0x3};
  ASSERT_THAT(clientChannel->sendMessage(message), IsTrue());

  message = {0x4, 0x5, 0x6};
  ASSERT_THAT(clientChannel->sendMessage(message), IsTrue());

  message = {0x7, 0x8, 0x9};
  ASSERT_THAT(serverChannel->sendMessage(message), IsTrue());

  message = {0xA, 0xB, 0xC};
  ASSERT_THAT(serverChannel->sendMessage(message), IsTrue());

  // Receive messages

  MessageChannel::ReceivedMessage result;

  spin();

  result = receiveMessage(serverChannel);
  ASSERT_THAT(result.success, IsTrue());
  ASSERT_THAT(result.message, ElementsAre(0x1, 0x2, 0x3));

  spin();

  result = receiveMessage(serverChannel);
  ASSERT_THAT(result.success, IsTrue());
  ASSERT_THAT(result.message, ElementsAre(0x4, 0x5, 0x6));

  spin();

  result = receiveMessage(serverChannel);
  ASSERT_THAT(result.success, IsTrue());
  ASSERT_THAT(result.message, IsEmpty());

  spin();

  result = receiveMessage(clientChannel);
  ASSERT_THAT(result.success, IsTrue());
  ASSERT_THAT(result.message, ElementsAre(0x7, 0x8, 0x9));

  spin();

  result = receiveMessage(clientChannel);
  ASSERT_THAT(result.success, IsTrue());
  ASSERT_THAT(result.message, ElementsAre(0xA, 0xB, 0xC));

  spin();

  result = receiveMessage(clientChannel);
  ASSERT_THAT(result.success, IsTrue());
  ASSERT_THAT(result.message, IsEmpty());
}

TEST_F(UnixSocketMsgChannel, ServerRestart) {
  ASSERT_THAT(waitForConnection(), IsTrue());

  ASSERT_THAT(serverChannel->isConnected(), IsTrue());
  ASSERT_THAT(clientChannel->isConnected(), IsTrue());

  serverChannel.reset();

  spin();

  // Client can immediately detect a disconnection because the socket file is
  // missing!
  ASSERT_THAT(clientChannel->isConnected(), IsFalse());

  // Should not be allowed to write anything while disconnected.
  std::vector<uint8_t> message = {0x1, 0x2, 0x3};
  ASSERT_THAT(clientChannel->sendMessage(message), IsFalse());

  initializeServerChannel();

  ASSERT_THAT(waitForConnection(), IsTrue());

  ASSERT_THAT(serverChannel->isConnected(), IsTrue());
  ASSERT_THAT(clientChannel->isConnected(), IsTrue());
}

TEST_F(UnixSocketMsgChannel, ClientRestart) {
  ASSERT_THAT(waitForConnection(), IsTrue());

  ASSERT_THAT(serverChannel->isConnected(), IsTrue());
  ASSERT_THAT(clientChannel->isConnected(), IsTrue());

  clientChannel.reset();

  spin();

  // Server cannot detect a disconnection until it tries to write.
  std::vector<uint8_t> message = {0x1, 0x2, 0x3};
  ASSERT_THAT(serverChannel->sendMessage(message), IsFalse());

  ASSERT_THAT(serverChannel->isConnected(), IsFalse());

  initializeClientChannel();

  ASSERT_THAT(waitForConnection(), IsTrue());

  ASSERT_THAT(serverChannel->isConnected(), IsTrue());
  ASSERT_THAT(clientChannel->isConnected(), IsTrue());
}
