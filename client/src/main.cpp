#include "commands/redraw.hpp"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include <netdb.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/ioctl.h>

#include "log.hpp"
#include "network.hpp"
#include "wifi.hpp"

static const char *TAG = "Main";

void blocking_checkin(int *sockfd) {
  while (true) {
    if (checkin(sockfd) != 0) {
      ESP_LOGE(TAG, "Check-in failed");
    } else {
      break;
    }

    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

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
  init_redraw();

  int sockfd;
  blocking_checkin(&sockfd);

  // The buffer is allocated and resized in parse_tcp_message automatically.
  uint32_t buffer_size = 0;
  uint8_t *buffer = nullptr;

  while (true) {
    if (parse_tcp_message(sockfd, &buffer, &buffer_size) < 0) {
      ESP_LOGI(TAG, "Reconnecting to server...");

      vTaskDelay(pdMS_TO_TICKS(CHECK_IN_DELAY_MS));

      blocking_checkin(&sockfd);
    }

    // unsigned long pending = 0;
    // if (ioctl(sockfd, FIONREAD, &pending) == 0) {
    //   printf("TCP RX buffer has %lu bytes pending\n", pending);
    // } else {
    //   perror("ioctl(FIONREAD)");
    // }

    // To prevent the task watchdog timer.
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  free(buffer);
}
