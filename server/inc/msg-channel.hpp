#pragma once

#include <cstdint>
#include <functional>
#include <span>

class MessageChannel {
public:
  enum Side { Client, Server };

  virtual ~MessageChannel() = default;

  virtual void process() {}

  /**
   * Send a message.
   *
   * @param message The message to send.
   * @return True if successful, false otherwise.
   */
  virtual bool sendMessage(std::span<const uint8_t> message) = 0;

  struct ReceivedMessage {
    bool success;
    std::span<const uint8_t> message;
  };

  /**
   * Receive a message. If no message is available, an empty message will be
   * returned.
   *
   * When called, this will signal to the message channel that the message has
   * been consumed, so the next process() call may invalidate the data pointed
   * to by the returned span. If the message data needs to persist longer then
   * it must be copied.
   *
   * Not thread-safe.
   *
   * @return A struct with a bool indicating success and a span containing the
   *         received message.
   */
  virtual ReceivedMessage receiveMessage() = 0;

  /**
   * Returns whether the receiver is currently connected.
   *
   * @return True or false
   */
  virtual bool isConnected() const = 0;

  using ConnectionCallback = std::function<void(bool)>;

  /**
   * Set a callback to be called when the receiver connects or disconnects.
   *
   * @param callback The callback to call when the receiver connects or
   * disconnects.
   */
  virtual void setConnectionCallback(ConnectionCallback callback) = 0;

  void clearConnectionCallback() { setConnectionCallback(nullptr); }

protected:
  MessageChannel() = default;
};
