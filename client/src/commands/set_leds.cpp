#include "esp_log.h"
#include "led_strip.h"
#include "protocol.hpp"
#include "redraw.hpp"
#include "set_config.hpp"
#include "hub75.h"

extern Hub75Driver *dma_display;
extern ImageEncoding image_encoding;

static const char *TAG = "SetLeds";

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
    uint8_t num_matrices = eh->num_matrices;
    p += sizeof(LEDsBatchEntryHeader);

    if (gpio_pin < 0) {
      if (dma_display) {
        uint32_t seen_leds = 0;
        for (uint8_t matIdx = 0; matIdx < num_matrices; matIdx++) {
          const LEDsPixelData* mat_pixel_data = reinterpret_cast<const LEDsPixelData*>(p);
          uint32_t width = mat_pixel_data->width;
          uint32_t height = mat_pixel_data->height;

          const uint8_t* head = mat_pixel_data->pixel_data;
          uint8_t bit = 0;

          for (uint32_t matPixelIdx = 0; matPixelIdx < width * height; ++matPixelIdx) {
            uint32_t idx = seen_leds + matPixelIdx;

            const Pixel pixel = decode_pixel(head, bit, matPixelIdx, mat_pixel_data->pixel_data,
                                             width, height, image_encoding);
            const Pixel rgb_pixel = pixel.toRGB();

            int panel_index = (-gpio_pin) - 1; 
            int x = (idx % 64) + (panel_index * 64); 
            int y = idx / 64;
            dma_display->set_pixel(x, y, rgb_pixel.R(), rgb_pixel.G(), rgb_pixel.B());
          }

          p += sizeof(LEDsPixelData) + get_encoded_image_size(width, height, image_encoding);
        }
      }
    } else {
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

      uint32_t seen_leds = 0;
      for (uint8_t matIdx = 0; matIdx < num_matrices; matIdx++) {
        const LEDsPixelData* mat_pixel_data = reinterpret_cast<const LEDsPixelData*>(p);
        uint32_t width = mat_pixel_data->width;
        uint32_t height = mat_pixel_data->height;

        const uint8_t* head = mat_pixel_data->pixel_data;
        uint8_t bit = 0;

        for (uint32_t matPixelIdx = 0; matPixelIdx < width * height; ++matPixelIdx) {
          uint32_t idx = seen_leds + matPixelIdx;

          const Pixel pixel = decode_pixel(head, bit, matPixelIdx, mat_pixel_data->pixel_data,
                                           width, height, image_encoding);
          const Pixel rgb_pixel = pixel.toRGB();

          ESP_ERROR_CHECK(led_strip_set_pixel(strip, idx, rgb_pixel.R(), rgb_pixel.G(), rgb_pixel.B()));
        }

        p += sizeof(LEDsPixelData) + get_encoded_image_size(width, height, image_encoding);
      }
    }
  }

  // TODO: ideally redraw cmd would be separate
  xTaskNotifyGive(notify_handle);

  return 0;
}
