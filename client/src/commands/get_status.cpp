#include "esp_log.h"
#include "get_status.hpp"
#include "protocol.hpp"

static const char *TAG = "GetStatus";

int get_status(GetStatusMessage *msg) {
  ESP_LOGI(TAG, "Handling get_status");
  send_status();

  return 0;
}

void send_status() {}
