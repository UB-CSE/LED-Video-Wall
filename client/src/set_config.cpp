#include "driver/gpio.h"
#include "led_strip.h"
#include "protocol.hpp"
#include "set_config.hpp"
#include <Arduino.h>
#include <map>

std::map<uint8_t, led_strip_handle_t> pin_to_handle;

void clear_led_strips() {
  for (auto &entry : pin_to_handle) {
    led_strip_del(entry.second);
  }
  pin_to_handle.clear();
}

void set_config(SetConfigMessage *msg) {
  if (msg == NULL) {
    Serial.println("Error: Null set_config message pointer");
    return;
  }

  Serial.println("Handling set_config");

  clear_led_strips();

  // uint16_t brightness = msg->init_brightness;
  uint8_t num_pins = msg->pins_used;

  if (num_pins == 0) {
    Serial.println("Error: num_pins cannot be zero");
    return;
  }

  for (int i = 0; i < num_pins; i++) {
    PinInfo *pinfo = &msg->pin_info[i];
    uint8_t gpio_pin = pinfo->pin_num;
    uint16_t num_leds = pinfo->max_leds;

    if (num_leds == 0) {
      Serial.printf("Warning: num_leds is zero for pin %d\n", gpio_pin);
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

    // TODO: read more on rmt vs spi and also dma here:
    // https://components.espressif.com/components/espressif/led_strip/versions/3.0.0
    led_strip_spi_config_t spi_config = {.clk_src = SPI_CLK_SRC_DEFAULT,
                                         .spi_bus = SPI2_HOST,
                                         .flags = {
                                             .with_dma = true,
                                         }};

    led_strip_handle_t strip;
    esp_err_t ret =
        led_strip_new_spi_device(&strip_config, &spi_config, &strip);
    if (ret != ESP_OK) {
      Serial.printf("Error: Failed to create LED strip for pin %d\n", gpio_pin);
      clear_led_strips();
      return;
    }

    led_strip_clear(strip);
    pin_to_handle[gpio_pin] = strip;
  }

  Serial.println("Configuration updated.");
}
