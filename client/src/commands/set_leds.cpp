#include "esp_log.h"
#include "led_strip.h"
#include "protocol.hpp"
#include "redraw.hpp"
#include "set_config.hpp"

static const char *TAG = "SetLeds";

int set_leds(SetLedsMessage *msg) {
  ESP_LOGI(TAG, "Handling set_leds");

  if (msg == NULL) {
    ESP_LOGE(TAG, "Invalid set_leds message (null)");
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

  return 0;
}

int set_leds_batched(SetLedsBatchedMessage *msg) {
  ESP_LOGI(TAG, "Handling set_leds_batched");

  if (msg == NULL) {
    ESP_LOGE(TAG, "Invalid set_leds_batched message (null)");
    return -1;
  }

  uint32_t total_size = msg->header.size;
  uint8_t batch_count = msg->batch_count;
  uint8_t *p = (uint8_t *)msg + sizeof(MessageHeader) + 1;
  uint8_t *end = (uint8_t *)msg + total_size;

  for (uint8_t i = 0; i < batch_count; ++i) {
    if (p + sizeof(LedsBatchEntryHeader) > end) {
      ESP_LOGE(TAG, "Batch %d is being read past all %d batches", i,
               batch_count);
      return -1;
    }

    LedsBatchEntryHeader *eh = (LedsBatchEntryHeader *)p;
    uint8_t gpio_pin = eh->gpio_pin;
    uint32_t num_leds = eh->num_leds;
    p += sizeof(LedsBatchEntryHeader);

    uint32_t pixel_bytes = num_leds * 3;
    if (p + pixel_bytes > end) {
      ESP_LOGE(
          TAG,
          "Batch %d has num leds %u and extends beyond the size of the message",
          *p, (unsigned int)num_leds);
      return -1;
    }

    auto it = pin_to_handle.find(gpio_pin);
    if (it == pin_to_handle.end()) {
      ESP_LOGE(TAG, "Unconfigured GPIO pin %d in batch %d", gpio_pin, i);
      return -1;
    }

    led_strip_handle_t strip = it->second;
    if (!strip) {
      ESP_LOGE(TAG, "LED strip handle not initialized for pin %d", gpio_pin);
      return -1;
    }

    for (uint32_t idx = 0; idx < num_leds; ++idx) {
      uint8_t r = p[idx * 3 + 0];
      uint8_t g = p[idx * 3 + 1];
      uint8_t b = p[idx * 3 + 2];
      ESP_ERROR_CHECK(led_strip_set_pixel(strip, idx, r, g, b));
    }

    p += pixel_bytes;
  }

  // TODO: ideally redraw cmd would be separate
  xTaskNotifyGive(notify_handle);

  return 0;
}
