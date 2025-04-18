#include "esp_log.h"
#include "led_strip.h"
#include "protocol.hpp"
#include "set_config.hpp"

static const char *TAG = "SetLeds";

int set_leds(SetLedsMessage *msg) {
  ESP_LOGI(TAG, "Handling set_leds");

  if (msg == NULL) {
    ESP_LOGW(TAG, "Invalid set_leds message (null)");
    return -1;
  }

  uint8_t gpio_pin = msg->gpio_pin;

  auto it = pin_to_handle.find(gpio_pin);
  if (it == pin_to_handle.end()) {
    ESP_LOGE(TAG, "Received data for an unconfigured GPIO pin %d.", gpio_pin);
    return -1;
  }

  led_strip_handle_t strip = it->second;
  if (!strip) {
    ESP_LOGE(TAG, "LED strip handle not initialized for pin %d", gpio_pin);
    return -1;
  }

  uint32_t data_size = msg->header.size - sizeof(SetLedsMessage);
  int num_pixels = data_size / 3; // assuming RGB
  uint8_t *pixel_data = msg->pixel_data;

  for (int i = 0; i < num_pixels; i++) {
    uint8_t r = pixel_data[i * 3];
    uint8_t g = pixel_data[i * 3 + 1];
    uint8_t b = pixel_data[i * 3 + 2];

    ESP_ERROR_CHECK(led_strip_set_pixel(strip, i, r, g, b));
  }

  // TODO: temp
  xTaskCreatePinnedToCore(
      [](void *param) {
        led_strip_handle_t strip = (led_strip_handle_t)param;
        ESP_ERROR_CHECK(led_strip_refresh(strip));
        vTaskDelete(NULL);
      },
      "LED Refresh", 2048, (void *)strip, 1, NULL, 1);

  return 0;
}
