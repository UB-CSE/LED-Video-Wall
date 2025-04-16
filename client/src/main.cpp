#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <Arduino.h>

#include "network.hpp"

static const char *TAG = "Main";

extern "C" void app_main(void) {
  initArduino();

  // TODO: move this to the loop, and make its behavior simialr to send_checkin
  connect_wifi();

  WiFiClient socket;
  send_checkin(socket);

  // The buffer is allocated and resized in parse_tcp_message automatically.
  uint32_t buffer_size = 0;
  uint8_t *buffer = nullptr;

  while (true) {
    if (!socket.connected()) {
      ESP_LOGI(TAG, "Reconnecting to server...");
      send_checkin(socket);
      vTaskDelay(pdMS_TO_TICKS(CHECK_IN_DELAY_MS));
      continue;
    }

    if (socket.available()) {
      parse_tcp_message(socket, &buffer, &buffer_size);
    }

    // To prevent the task watchdog timer.
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
