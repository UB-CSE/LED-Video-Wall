#include "esp_log.h"

#include <stdlib.h>
#include <sys/socket.h>

#include "log.hpp"
#include "protocol.hpp"

static const char *TAG = "GetLogs";

int send_logs(int sockfd) {
  char *logs = NULL;
  size_t len = 0;
  int ret = get_buffered_logs(&logs, &len);
  if (ret != 0) {
    ESP_LOGE(TAG, "Failed to get buffered logs");
    return -1;
  }

  uint32_t message_len = 0;
  uint8_t *message = encode_send_logs((const char *)logs, &message_len);
  free(logs);

  if (!message) {
    ESP_LOGE(TAG, "Failed to encode message");
    return -1;
  }

  size_t sent = 0;
  while (sent < message_len) {
    // TODO: this stuff should probably be offloaded to a separate task
    ssize_t bytes_read = send(sockfd, message + sent, message_len - sent, 0);
    // TODO: does this have the same issue as socket read in network.cpp?
    if (bytes_read <= 0) {
      ESP_LOGW(TAG, "Failed to send socket: %d", errno);
      free_message_buffer(message);
      return -1;
    }

    sent += (size_t)bytes_read;
  }

  free_message_buffer(message);

  return 0;
}

int get_logs(GetLogsMessage *msg, int sockfd) {
  ESP_LOGI(TAG, "Handling get_logs");

  if (msg == NULL) {
    ESP_LOGE(TAG, "Invalid set_leds message (null)");
    return -1;
  }

  if (send_logs(sockfd) != 0) {
    ESP_LOGE(TAG, "Failed to send logs");
    return -1;
  }

  return 0;
}
