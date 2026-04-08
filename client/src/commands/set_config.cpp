#include "esp_log.h"
#include "led_strip.h"

#include <map>

#include "protocol.hpp"
#include "set_config.hpp"
#include "hub75.h"
Hub75Driver *dma_display = nullptr;

static const char *TAG = "SetConfig";

std::map<int8_t, led_strip_handle_t> pin_to_handle;
SemaphoreHandle_t pin_to_handle_mutex = xSemaphoreCreateMutex();

void clear_led_strips() {
  for (auto &entry : pin_to_handle) {
    led_strip_del(entry.second);
  }
  pin_to_handle.clear();
}

int set_config(SetConfigMessage *msg) {
  ESP_LOGI(TAG, "Handling set_config");

  if (msg == NULL) {
    ESP_LOGE(TAG, "Invalid set_config message (null)");
    return -1;
  }

  xSemaphoreTake(pin_to_handle_mutex, portMAX_DELAY);
  clear_led_strips();

  uint8_t num_pins = msg->pins_used;

  if (num_pins == 0) {
    ESP_LOGE(TAG, "num_pins cannot be zero");
    xSemaphoreGive(pin_to_handle_mutex);
    return -1;
  }

  int p3_panel_count = 0;
  for (int i = 0; i < num_pins; i++) {
    if (msg->pin_info[i].pin_num < 0) {
      p3_panel_count++;
    }
  }

 if (p3_panel_count > 0 && !dma_display) {
    Hub75Config config{};
    config.panel_width = 64; 
    config.panel_height = 64;
    //config.double_buffer = true;
    config.layout_cols = p3_panel_count;
    config.layout_rows = 1;
    config.output_clock_speed = Hub75ClockSpeed::HZ_10M;
    config.pins.r1 = 42; config.pins.g1 = 41; config.pins.b1 = 40;
    config.pins.r2 = 38; config.pins.g2 = 39; config.pins.b2 = 37;
    config.pins.a = 45; config.pins.b = 36; config.pins.c = 48; config.pins.d = 35; config.pins.e = 21;
    config.pins.lat = 47; config.pins.oe = 14; config.pins.clk = 2;
    dma_display = new Hub75Driver(config);
    dma_display->begin();
  }

  for (int i = 0; i < num_pins; i++) {
    PinInfo *pinfo = &msg->pin_info[i];
    int8_t gpio_pin = pinfo->pin_num;
    if (gpio_pin < 0) {
      continue;
    }
    uint16_t num_leds = pinfo->max_leds;

    if (num_leds == 0) {
      ESP_LOGE(TAG, "num_leds is zero for pin %d", (unsigned int)gpio_pin);
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
        .clk_src = RMT_CLK_SRC_APB,
        .resolution_hz = 80 * 1000 * 1000,
        // TODO: assuming 4 matrices on esp32
        .mem_block_symbols = 128,
        // TODO: read more on rmt vs spi and also dma here:
        //
        // https : //
        // components.espressif.com/components/espressif/led_strip/versions/3.0.0
        .flags = {.with_dma = false},
    };

    led_strip_handle_t strip;
    esp_err_t ret =
        led_strip_new_rmt_device(&strip_config, &rmt_config, &strip);
    if (ret != ESP_OK) {
      ESP_LOGE(TAG, "Failed to create LED strip for pin %d",
               (unsigned int)gpio_pin);
      clear_led_strips();
      xSemaphoreGive(pin_to_handle_mutex);
      return -1;
    }

    ESP_ERROR_CHECK(led_strip_clear(strip));
    pin_to_handle[gpio_pin] = strip;
  }

  ESP_LOGI(TAG, "Configuration updated");

  xSemaphoreGive(pin_to_handle_mutex);

  return 0;
}
