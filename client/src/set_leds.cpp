#include "led_strip.h"
#include "protocol.hpp"
#include "set_config.hpp"
#include <Arduino.h>

void set_leds(SetLedsMessage *msg) {
  Serial.println("Handling set_leds");

  if (!msg) {
    Serial.println("Error: Invalid set_leds message (null)");
    return;
  }

  uint8_t gpio_pin = msg->gpio_pin;

  auto it = pin_to_handle.find(gpio_pin);
  if (it == pin_to_handle.end()) {
    Serial.printf("Error: Received data for an unconfigured GPIO pin (%d).\n",
                  gpio_pin);
    return;
  }

  led_strip_handle_t strip = it->second;
  if (!strip) {
    Serial.printf("Error: LED strip handle not initialized for pin %d\n",
                  gpio_pin);
    return;
  }

  uint32_t data_size = msg->header.size - sizeof(SetLedsMessage);
  int num_pixels = data_size / 3; // assuming RGB
  uint8_t *pixel_data = msg->pixel_data;

  for (int i = 0; i < num_pixels; i++) {
    uint8_t r = pixel_data[i * 3];
    uint8_t g = pixel_data[i * 3 + 1];
    uint8_t b = pixel_data[i * 3 + 2];

    led_strip_set_pixel(strip, i, r, g, b);
  }

  // TODO: temp
  led_strip_refresh(strip);
}
