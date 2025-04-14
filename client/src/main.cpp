#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <Arduino.h>

#include "network.hpp"

static const char *TAG = "Main";

extern "C" void app_main(void) {
  initArduino();

  Serial.begin(115200);

  connect_wifi();

  WiFiClient socket;
  send_checkin(socket);

  while (true) {
    if (!socket.connected()) {
      ESP_LOGI(TAG, "Reconnecting to server...");
      send_checkin(socket);
      vTaskDelay(pdMS_TO_TICKS(CHECK_IN_DELAY_MS));
      return;
    }

    if (socket.available()) {
      parse_tcp_message(socket);
    }

    // To prevent the task watchdog timer.
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
