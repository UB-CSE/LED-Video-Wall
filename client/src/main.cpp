#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <Arduino.h>

#include "network.hpp"

static const char *TAG = "Main";

void setup() {
  Serial.begin(115200);
  connect_wifi();
}

void loop() {
  if (!socket.connected()) {
    ESP_LOGI(TAG, "Reconnecting to server...");
    send_checkin();
    vTaskDelay(pdMS_TO_TICKS(CHECK_IN_DELAY_MS));
    return;
  }

  if (socket.available()) {
    parse_tcp_message();
  }

  // To prevent the task watchdog timer.
  vTaskDelay(pdMS_TO_TICKS(10));
}
