#include "ota.hpp"
#include "esp_check.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

#define TAG "OTA"

// helper to download a file into buffer
static esp_err_t http_download(const char *url, char *buf, size_t buf_sz,
                               int *out_len) {
  esp_http_client_config_t cfg = {
      .url = url,
      .timeout_ms = 5000,
  };
  esp_http_client_handle_t http_client = esp_http_client_init(&cfg);
  if (!http_client)
    return ESP_FAIL;

  esp_err_t err = esp_http_client_open(http_client, 0);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "error opening http client - %s", esp_err_to_name(err));
    return err;
  }

  int content_length = esp_http_client_fetch_headers(http_client);
  if (content_length <= 0 || content_length >= buf_sz) {
    esp_http_client_close(http_client);
    esp_http_client_cleanup(http_client);
    return ESP_FAIL;
  }

  int offset = 0;
  while (offset < content_length) {
    int read =
        esp_http_client_read(http_client, buf + offset, buf_sz - 1 - offset);
    if (read <= 0)
      break; // connection closed early
    offset += read;
  }
  buf[offset] = '\0';
  if (out_len)
    *out_len = offset;

  esp_http_client_close(http_client);
  esp_http_client_cleanup(http_client);

  if (offset > 0) {
    return ESP_OK;
  } else {
    return ESP_FAIL;
  }
}

static esp_err_t ota_fail(esp_ota_handle_t h,
                          esp_http_client_handle_t http_client) {
  esp_http_client_close(http_client);
  esp_http_client_cleanup(http_client);
  esp_ota_abort(h);
  return ESP_FAIL;
}

// helper to download OTA binary, load it, and restart
static esp_err_t do_http_ota(const char *url) {
  const esp_partition_t *next = esp_ota_get_next_update_partition(NULL);
  if (!next) {
    ESP_LOGE(TAG, "No OTA partition");
    return ESP_FAIL;
  }

  esp_ota_handle_t ota_handle = 0;
  ESP_RETURN_ON_ERROR(esp_ota_begin(next, OTA_SIZE_UNKNOWN, &ota_handle), TAG,
                      "ota_begin");

  esp_http_client_config_t cfg = {.url = url};
  esp_http_client_handle_t http_client = esp_http_client_init(&cfg);
  ESP_RETURN_ON_FALSE(http_client, ESP_FAIL, TAG, "http init");

  ESP_RETURN_ON_ERROR(esp_http_client_open(http_client, 0), TAG, "http open");

  // must fetch headers before getting status, otherwise status will be invalid
  int content_len = esp_http_client_fetch_headers(http_client);
  int status = esp_http_client_get_status_code(http_client);

  if (status != 200) {
    ESP_LOGE(TAG, "HTTP status not OK (received %d) - aborting OTA", status);
    return ota_fail(ota_handle, http_client);
  }

  if (content_len <= OTA_MIN_VALID_IMAGE_SIZE) {
    ESP_LOGE(TAG, "Unexpected content length (%d) - incorrect firmware binary?",
             content_len);
    return ota_fail(ota_handle, http_client);
  }

  char buf[2048];
  int read;
  while ((read = esp_http_client_read(http_client, buf, sizeof(buf))) > 0) {
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_ota_write(ota_handle, buf, read));
  }
  if (read < 0) {
    ESP_LOGE(TAG, "HTTP read error");
    return ota_fail(ota_handle, http_client);
  }

  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_ota_end(ota_handle));
  ESP_ERROR_CHECK_WITHOUT_ABORT(esp_ota_set_boot_partition(next));

  ESP_LOGI(TAG, "OTA SUCCESS - rebooting");
  esp_restart();

  // Never will reach as the client should restart.
  return ESP_OK;
}

static void ota_task(void *arg) {
  char remote_ver[32];
  int remote_len = 0;

  if (http_download(VERSION_URL, remote_ver, sizeof(remote_ver), &remote_len) !=
          ESP_OK ||
      remote_len <= 0) {
    ESP_LOGE(TAG, "Could not fetch version.txt");
    vTaskDelete(NULL);
    return;
  }

  ESP_LOGI(TAG, "Remote version: \"%s\"", remote_ver);
  ESP_LOGI(TAG, "Local version: \"%s\"", CURRENT_FW_VERSION);

  if (strcmp(remote_ver, CURRENT_FW_VERSION) == 0) {
    ESP_LOGI(TAG, "Already up-to-date");
  } else {
    ESP_LOGI(TAG, "New firmware detected - starting OTA");
    do_http_ota(FW_URL); // should restart the microcontroller on success
  }
  vTaskDelete(NULL);
}

/* Starts the actual OTA task. We need to task rather than running ota_task
 * function directly in order to ensure a large enough stack is allocated.
 * */
void start_ota_checker(void) {
  xTaskCreatePinnedToCore(ota_task, "ota_task", 8192, NULL, 5, NULL,
                          APP_CPU_NUM);
}
