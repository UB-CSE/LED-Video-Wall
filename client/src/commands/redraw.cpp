#include "esp_log.h"
#include "led_strip.h"

#include "protocol.hpp"
#include "redraw.hpp"
#include "set_config.hpp"

static const char *TAG = "Redraw";

int redraw(RedrawMessage *msg) {
  ESP_LOGI(TAG, "Handling redraw");

  if (msg == NULL) {
    ESP_LOGW(TAG, "Invalid redraw message (null)");
    return -1;
  }

  if (pin_to_handle.empty()) {
    ESP_LOGE(TAG, "No LED strips configured");
    return -1;
  }

  for (auto &entry : pin_to_handle) {
    // "What's worse, if the RMT interrupt is delayed or not serviced in time
    // (e.g. if Wi-Fi interrupt happens on the same CPU core), the RMT
    // transaction will be corrupted and the LEDs will display incorrect
    // colors." I'm not sure if it's necessary to pin the task to a core, but ^
    // is found in the docs and WiFi is on core 0.
    auto strip = entry.second;
    xTaskCreatePinnedToCore(
        [](void *param) {
          led_strip_handle_t s = (led_strip_handle_t)param;
          led_strip_refresh(s);
          vTaskDelete(nullptr);
        },
        "LED Refresh", 2048, (void *)strip, 1, nullptr, 1);
  }

  ESP_LOGI(TAG, "LEDs refreshed across all configured pins.");

  return 0;
}
