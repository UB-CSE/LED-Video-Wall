#include "esp_log.h"
#include "led_strip.h"

#include "protocol.hpp"
#include "redraw.hpp"
#include "set_config.hpp"

static const char *TAG = "Redraw";

TaskHandle_t notify_handle = nullptr;

IRAM_ATTR static void redraw_task(void *) {
  while (true) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    xSemaphoreTake(pin_to_handle_mutex, portMAX_DELAY);
    for (auto &entry : pin_to_handle) {
      led_strip_handle_t strip = entry.second;
      // ESP_LOGD(TAG, "Refreshing strip on pin %d", entry.first);
      ESP_ERROR_CHECK(led_strip_refresh(strip));
    }
    xSemaphoreGive(pin_to_handle_mutex);

    // ESP_LOGI(TAG, "Completed full LED redraw.");
  }
}

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

  xTaskNotifyGive(notify_handle);

  return 0;
}

void init_redraw() {
  // "What's worse, if the RMT interrupt is delayed or not serviced in time
  // (e.g. if Wi-Fi interrupt happens on the same CPU core), the RMT
  // transaction will be corrupted and the LEDs will display incorrect
  // colors." I'm not sure if it's necessary to pin the task to a core, but ^
  // is found in the docs and WiFi is on core 0.
  xTaskCreatePinnedToCore(redraw_task, "LED_Redraw", 4096, nullptr, 2,
                          &notify_handle, 1);

  ESP_LOGI(TAG, "Started redraw task on core 1");
}
