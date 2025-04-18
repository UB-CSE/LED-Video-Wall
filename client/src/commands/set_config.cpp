#include "esp_log.h"
#include "led_strip.h"

#include <map>

#include "protocol.hpp"
#include "set_config.hpp"

static const char *TAG = "SetConfig";

std::map<uint8_t, led_strip_handle_t> pin_to_handle;

void clear_led_strips() {
  for (auto &entry : pin_to_handle) {
    led_strip_del(entry.second);
  }
  pin_to_handle.clear();
}

int set_config(SetConfigMessage *msg) {
  ESP_LOGI(TAG, "Handling set_config");

  if (msg == NULL) {
    ESP_LOGW(TAG, "Invalid set_config message (null)");
    return -1;
  }

  clear_led_strips();

  uint8_t num_pins = msg->pins_used;

  if (num_pins == 0) {
    ESP_LOGE(TAG, "num_pins cannot be zero");
    return -1;
  }

  for (int i = 0; i < num_pins; i++) {
    PinInfo *pinfo = &msg->pin_info[i];
    uint8_t gpio_pin = pinfo->pin_num;
    uint16_t num_leds = pinfo->max_leds;

    if (num_leds == 0) {
      ESP_LOGE(TAG, "num_leds is zero for pin %u", (unsigned int)gpio_pin);
      continue;
    }

    led_strip_config_t strip_config = {
        .strip_gpio_num = gpio_pin,
        .max_leds = num_leds,
        // TODO: handle led_type field
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags =
            {
                .invert_out = false,
            },
    };

    // TODO: check if dma is supported
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        // TODO: read more on rmt vs spi and also dma here:
        // https://components.espressif.com/components/espressif/led_strip/versions/3.0.0
        .flags = {.with_dma = false},
    };

    led_strip_handle_t strip;
    esp_err_t ret =
        led_strip_new_rmt_device(&strip_config, &rmt_config, &strip);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to create LED strip for pin %u",
               (unsigned int)gpio_pin);
      clear_led_strips();
      return -1;
    }

    ESP_ERROR_CHECK(led_strip_clear(strip));
    pin_to_handle[gpio_pin] = strip;
  }

  ESP_LOGI(TAG, "Configuration updated");

  return 0;
}
