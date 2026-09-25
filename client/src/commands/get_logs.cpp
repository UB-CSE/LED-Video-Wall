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

  std::vector<uint8_t> message = encode_send_logs((const char *)logs);
  free(logs);

  if (message.empty()) {
    ESP_LOGE(TAG, "Failed to encode message");
    return -1;
  }

  size_t sent = 0;
  while (sent < message.size()) {
    // TODO: this stuff should probably be offloaded to a separate task
    ssize_t bytes_read = send(sockfd, message.data() + sent, message.size() - sent, 0);
    // TODO: does this have the same issue as socket read in network.cpp?
    if (bytes_read <= 0) {
      ESP_LOGW(TAG, "Failed to send socket: %d", errno);
      return -1;
    }

    sent += (size_t)bytes_read;
  }

  return 0;
}

int get_logs(const GetLogsMessage *msg, int sockfd) {
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
