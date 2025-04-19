#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include <errno.h>
#include <netdb.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include "log.hpp"
#include "network.hpp"
#include "wifi.hpp"

static const char *TAG = "Main";

extern "C" void app_main(void) {
  init_buffered_log();

  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
      ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  init_wifi();

  int sockfd;
  if (checkin(&sockfd) != 0) {
    ESP_LOGE(TAG, "Check-in failed");
  }

  // The buffer is allocated and resized in parse_tcp_message automatically.
  uint32_t buffer_size = 0;
  uint8_t *buffer = nullptr;

  while (true) {
    int available = 0;
    if (ioctl(sockfd, FIONREAD, &available) < 0) {
      ESP_LOGW(TAG, "Failed to check if data is available: %d", errno);
      available = 0;
    }

    if (available > 0) {
      if (parse_tcp_message(sockfd, &buffer, &buffer_size) < 0) {
        ESP_LOGI(TAG, "Reconnecting to server...");

        vTaskDelay(pdMS_TO_TICKS(CHECK_IN_DELAY_MS));

        if (checkin(&sockfd) != 0) {
          ESP_LOGE(TAG, "Check-in failed");
        }
      }
    }

    // To prevent the task watchdog timer.
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  free(buffer);
}
