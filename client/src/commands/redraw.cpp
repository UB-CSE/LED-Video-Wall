#include "esp_log.h"
#include "led_strip.h"

#include "protocol.hpp"
#include "redraw.hpp"
#include "set_config.hpp"

static const char *TAG = "Redraw";

void redraw(RedrawMessage *msg) {
  ESP_LOGI(TAG, "Handling redraw");

  if (msg == NULL) {
    ESP_LOGW(TAG, "Invalid redraw message (null)");
    return;
  }

  if (pin_to_handle.empty()) {
    ESP_LOGE(TAG, "No LED strips configured");
    return;
  }

  for (auto &entry : pin_to_handle) {
    led_strip_refresh(entry.second);
  }

  ESP_LOGI(TAG, "LEDs refreshed across all configured pins.");
}
