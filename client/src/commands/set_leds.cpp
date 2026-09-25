#include "esp_log.h"
#include "led_strip.h"
#include "protocol.hpp"
#include "redraw.hpp"
#include "set_config.hpp"
#include "hub75.h"

extern Hub75Driver *dma_display;
extern ImageEncoding image_encoding;

static const char *TAG = "SetLeds";

int set_leds(const SetLEDsMessage *msg) {
  ESP_LOGI(TAG, "Handling set_leds");

  if (msg == NULL) {
    ESP_LOGE(TAG, "Invalid set_leds message (null)");
    return -1;
  }

  int8_t gpio_pin = msg->header.gpio_pin;
  if (gpio_pin < 0) {
    if (!dma_display) return -1;
    int num_pixels = msg->header.num_leds;
    const uint8_t *pixel_data = msg->pixel_data;

    const uint8_t* head = pixel_data;
    uint8_t bit = 0;
    for (int i = 0; i < num_pixels; i++) {
      const Pixel pixel = decode_pixel(head, bit, i, image_encoding);
      const Pixel rgb_pixel = pixel.toRGB();

      int panel_index = (-gpio_pin) - 1;
      int x = (i % 64) + (panel_index * 64); 
      int y = i / 64;
      dma_display->set_pixel(x, y, rgb_pixel.R(), rgb_pixel.G(), rgb_pixel.B());
    }
    return 0;
  }

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

  int num_pixels = msg->header.num_leds;
  const uint8_t *pixel_data = msg->pixel_data;

  const uint8_t* head = pixel_data;
  uint8_t bit = 0;
  for (int i = 0; i < num_pixels; i++) {
    const Pixel pixel = decode_pixel(head, bit, i, image_encoding);
    const Pixel rgb_pixel = pixel.toRGB();

    ESP_ERROR_CHECK(led_strip_set_pixel(strip, i, rgb_pixel.R(), rgb_pixel.G(), rgb_pixel.B()));
  }

  return 0;
}

int set_leds_batched(const SetLEDsBatchedMessage *msg) {
  ESP_LOGI(TAG, "Handling set_leds_batched");

  if (msg == NULL) {
    ESP_LOGE(TAG, "Invalid set_leds_batched message (null)");
    return -1;
  }

  uint32_t total_size = msg->header.header.size;
  uint8_t batch_count = msg->header.batch_count;
  const uint8_t *p = (uint8_t *)msg->entries;
  const uint8_t *end = (uint8_t *)msg + total_size;

  for (uint8_t i = 0; i < batch_count; ++i) {
    const LEDsBatchEntry* entry = (const LEDsBatchEntry*)p;

    if (p + sizeof(LEDsBatchEntryHeader) > end) {
      ESP_LOGE(TAG, "Batch %d is being read past all %d batches", i,
               batch_count);
      return -1;
    }

    const LEDsBatchEntryHeader *eh = &entry->header;
    int8_t gpio_pin = eh->gpio_pin;
    uint32_t num_leds = eh->num_leds;
    uint32_t pixel_bytes = get_encoded_image_size(num_leds, image_encoding);
    p += sizeof(LEDsBatchEntryHeader);
    if (p + pixel_bytes > end) {
      ESP_LOGE(
          TAG,
          "Batch %d has num leds %u and extends beyond the size of the message",
          *p, (unsigned int)num_leds);
      return -1;
    }
    if (gpio_pin < 0) {
      if (dma_display) {
        const uint8_t* head = p;
        uint8_t bit = 0;
        for (uint32_t idx = 0; idx < num_leds; ++idx) {
          const Pixel pixel = decode_pixel(head, bit, idx, image_encoding);
          const Pixel rgb_pixel = pixel.toRGB();

          int panel_index = (-gpio_pin) - 1; 
          int x = (idx % 64) + (panel_index * 64); 
          int y = idx / 64;
          dma_display->set_pixel(x, y, rgb_pixel.R(), rgb_pixel.G(), rgb_pixel.B());
        }
      }
      p += pixel_bytes;
      continue;
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

    const uint8_t* head = p;
    uint8_t bit = 0;
    for (uint32_t idx = 0; idx < num_leds; ++idx) {
      const Pixel pixel = decode_pixel(head, bit, idx, image_encoding);
      const Pixel rgb_pixel = pixel.toRGB();
      ESP_ERROR_CHECK(led_strip_set_pixel(strip, idx, rgb_pixel.R(), rgb_pixel.G(), rgb_pixel.B()));
    }

    p += pixel_bytes;
  }

  // TODO: ideally redraw cmd would be separate
  xTaskNotifyGive(notify_handle);

  return 0;
}
