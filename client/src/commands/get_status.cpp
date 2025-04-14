#include "esp_log.h"
#include "get_status.hpp"
#include "protocol.hpp"
#include <Arduino.h>

static const char *TAG = "GetStatus";

void get_status(GetStatusMessage *msg) {
  ESP_LOGI(TAG, "Handling get_status");
  send_status();
}

void send_status() {}
