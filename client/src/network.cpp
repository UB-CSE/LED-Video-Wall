#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "commands/get_logs.hpp"
#include "commands/redraw.hpp"
#include "commands/set_config.hpp"
#include "commands/set_leds.hpp"
#include "network.hpp"
#include "protocol.hpp"

#include "esp_http_client.h"              // NEW: HTTP client for discovery

static const char *TAG = "Network";

// There should be a separate header file, "wifi_credentials.hpp", that defines
// preprocessor constants, WIFI_SSID and WIFI_PASSWORD, that are used by the
// clients to connect to wifi.
// !!! THE CREDENTIALS FILE MUST _NOT_ BE ADDDED TO GIT/SOURCE CONTROL !!!
// Here is an example of what "wifi_credentials.hpp" look like:
// -----------------------------------------------------------
// #ifndef CREDENTIALS_HPP
// #define CREDENTIALS_HPP
// #define WIFI_SSID "UB_Connect"
// #define WIFI_PASSWORD ""
// #endif
// -----------------------------------------------------------
#if __has_include("wifi_credentials.hpp")
#include "wifi_credentials.hpp"
#else
#warning "'wifi_credentials.hpp' not specified! Using default credentials."
#define WIFI_SSID "UB_Connect"
#define WIFI_PASSWORD ""
#endif

// NEW: URL that returns the server host/IP as plain text, e.g. "192.168.1.50"
#define DISCOVERY_URL "http://example.com/host"   // <-- change this to your URL

// NEW: Mutable server IP string; starts with compile-time SERVER_IP default
static char g_server_ip[INET_ADDRSTRLEN] = SERVER_IP;

// NEW: Minimal HTTP event handler (we don’t really need events here)
static esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
  return ESP_OK;
}

// NEW: Fetch server IP/host from HTTP endpoint and store in g_server_ip.
//      Returns 0 on success, -1 on error.
static int update_server_ip_from_http(void) {
  ESP_LOGI(TAG, "Fetching server IP from discovery URL: %s", DISCOVERY_URL);

  esp_http_client_config_t config = {};
  config.url = DISCOVERY_URL;
  config.method = HTTP_METHOD_GET;
  config.event_handler = _http_event_handler;

  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (client == nullptr) {
    ESP_LOGE(TAG, "Failed to initialize HTTP client");
    return -1;
  }

  esp_err_t err = esp_http_client_perform(client);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "HTTP GET failed: %s", esp_err_to_name(err));
    esp_http_client_cleanup(client);
    return -1;
  }

  // Read response body (assumes short response like "192.168.1.23\n")
  char buffer[64] = {0};
  int read_len = esp_http_client_read(client, buffer, sizeof(buffer) - 1);
  esp_http_client_cleanup(client);

  if (read_len <= 0) {
    ESP_LOGE(TAG, "Failed to read HTTP response body");
    return -1;
  }

  buffer[read_len] = '\0';  // ensure null-terminated

  // Strip trailing CR/LF and spaces
  char *p = buffer;
  while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) {
    ++p;  // skip leading whitespace (if any)
  }
  char *end = p + strlen(p);
  while (end > p && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n')) {
    *--end = '\0';
  }

  if (*p == '\0') {
    ESP_LOGE(TAG, "Discovery response was empty/whitespace");
    return -1;
  }

  // Copy into global server IP buffer
  strncpy(g_server_ip, p, sizeof(g_server_ip) - 1);
  g_server_ip[sizeof(g_server_ip) - 1] = '\0';

  ESP_LOGI(TAG, "Discovered server host/IP: '%s'", g_server_ip);
  return 0;
}

int read_exact(int sockfd, uint8_t *buffer, uint32_t len) {
  uint32_t total = 0;
  while (total < len) {
    ssize_t bytes_read = recv(sockfd, buffer + total, len - total, MSG_WAITALL);
    if (bytes_read > 0) {
      total += bytes_read;
      ESP_LOGD(TAG, "Read %d bytes, total read: %u/%u bytes", bytes_read,
               (unsigned int)total, (unsigned int)len);
    } else if (bytes_read == 0) {
      ESP_LOGW(TAG, "Socket disconnected");
      return -1;
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
      ESP_LOGW(TAG, "Error reading socket: %d; retrying", errno);
      vTaskDelay(1 / portTICK_PERIOD_MS);
      continue;
    } else {
      ESP_LOGW(TAG, "Error reading socket: %d", errno);
      return -1;
    }
  }

  return 0;
}

int checkin(int *out_sockfd) {
  ESP_LOGI(TAG, "Sending check-in message");

  // NEW: Try to update server IP via HTTP discovery before connecting
  if (update_server_ip_from_http() != 0) {
    ESP_LOGW(TAG, "Server discovery via HTTP failed, using default IP: %s",
             g_server_ip);
  }

  int sockfd = -1;
  struct sockaddr_in dest_addr;
  dest_addr.sin_family = AF_INET;

  // CHANGED: use g_server_ip (possibly discovered) instead of hard-coded SERVER_IP
  if (inet_pton(AF_INET, g_server_ip, &dest_addr.sin_addr.s_addr) != 1) {
    ESP_LOGE(TAG, "Invalid server IP/host string: %s", g_server_ip);
    return -1;
  }

  // When attempting to connect to the server, there are multiple ports that it
  // may have connected to. The preprocessor constants SERVER_PORT_START and
  // SERVER_PORT_END denote the range of ports, start and end included, that the
  // server may be using. The code below attempts to connect on all the ports in
  // the range, and simply returns if none of the ports resulted in a successful
  // connection. If a connection does succeed then the loop ends early and
  // continues with the rest of the code.
  for (uint16_t port = SERVER_PORT_START; port <= SERVER_PORT_END; port++) {
    if (sockfd >= 0) {
      close(sockfd);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sockfd < 0) {
      ESP_LOGE(TAG, "Failed to create socket: %d", errno);
      continue;
    }

    // TODO: if the server ever restarts or anything, it will take
    // RECV_TIMEOUT_SEC to timeout a read and reconnect. This isn't ideal, we
    // should instead use select/polling in the main loop
    struct timeval tv {
      RECV_TIMEOUT_SEC, 0
    };
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
      ESP_LOGE(TAG, "Failed to set socket recv timeout");
      return -1;
    }

    dest_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) ==
        0) {
      // CHANGED: Log g_server_ip instead of SERVER_IP
      ESP_LOGI(TAG, "Connected to %s:%u", g_server_ip, port);
      break;
    } else {
      ESP_LOGD(TAG, "Connect to port %u failed: %d", port, errno);
      close(sockfd);
      sockfd = -1;
    }
  }

  if (sockfd < 0) {
    ESP_LOGI(TAG, "Unable to connect to any server port");
    return -1;
  }

  uint8_t mac[6];
  ESP_ERROR_CHECK(esp_wifi_get_mac(WIFI_IF_STA, mac));

  uint32_t message_size = 0;
  uint8_t *buffer = encode_check_in(mac, &message_size);
  if (!buffer) {
    ESP_LOGW(TAG, "Failed to encode check-in message");
    close(sockfd);
    return -1;
  }

  ssize_t written = send(sockfd, buffer, message_size, 0);
  free_message_buffer(buffer);

  if (written < 0) {
    ESP_LOGE(TAG, "Send check-in failed: %d", errno);
    close(sockfd);
    return -1;
  }

  ESP_LOGI(TAG, "Check-in message sent");
  *out_sockfd = sockfd;

  return 0;
}

int parse_tcp_message(int sockfd, uint8_t **buffer, uint32_t *buffer_size) {
  uint8_t size_buffer[sizeof(uint32_t)];
  if (read_exact(sockfd, size_buffer, sizeof(size_buffer)) != 0) {
    return -1;
  }

  uint32_t message_size = get_message_size(size_buffer);

  ESP_LOGD(TAG, "Message size from header: %u bytes",
           (unsigned int)message_size);
  if (message_size == 0) {
    ESP_LOGE(TAG, "msg size of 0");
    // close(sockfd);
    return -1;
  } else if (message_size > 10000) {
    ESP_LOGE(TAG, "msg is way too big");
    // close(sockfd);
    return -1;
  }

  // The buffer is resized to the maximum size message we can possibly
  // receive. Since we expect to receive that same size message multiple times,
  // this is a fair optimization.
  //
  // We may additionally consider resizing this buffer depending on the maximum
  // constraints given from a call to set_config, otherwise we'd expect this to
  // resize rather significantly on the first call to set_leds.
  if (message_size > *buffer_size) {
    uint8_t *new_buffer = (uint8_t *)realloc(*buffer, message_size);
    if (new_buffer) {
      *buffer = new_buffer;
      *buffer_size = message_size;
    } else {
      ESP_LOGW(TAG, "Failed to resize message buffer");

      // If we fail to realloc the buffer, then we can't read the whole message.
      // If we return from this function and it finds bytes available again, it
      // may start reading in the middle of a message, resulting in undefined
      // behavior. Thus, we close the socket and return, allowing the client to
      // reconnect and resume later.
      // close(sockfd);

      return -1;
    }
  }

  memcpy(*buffer, size_buffer, sizeof(uint32_t));

  uint32_t remaining_bytes = message_size - sizeof(uint32_t);
  if (read_exact(sockfd, *buffer + sizeof(uint32_t), remaining_bytes) != 0) {
    return -1;
  }

  uint16_t op_code = get_message_op_code(*buffer);
  ESP_LOGD(TAG, "Received OpCode: 0x%04X", op_code);

  switch (op_code) {
  case OP_SET_LEDS: {
    if (set_leds(decode_set_leds(*buffer)) != 0) {
      return -1;
    }
    break;
  }
  case OP_SET_LEDS_BATCHED: {
    // int64_t time_us = esp_timer_get_time();

    // // Convert to seconds
    // double time_s = time_us / 1000000.0;

    // ESP_LOGI("TIME_SINCE_BOOT", "Time since boot: %.6f seconds", time_s);

    // int64_t t_start = esp_timer_get_time();
    if (set_leds_batched(decode_set_leds_batched(*buffer)) != 0) {
      return -1;
    }

    // int64_t t_end = esp_timer_get_time();
    // int64_t elapsed_us = t_end - t_start;
    // ESP_LOGI(TAG, "set_leds_batched completed in %lld µs", elapsed_us);
    break;
  }
  case OP_GET_LOGS: {
    if (get_logs(decode_get_logs(*buffer), sockfd) != 0) {
      return -1;
    }
    break;
  }
  case OP_REDRAW: {

    // int64_t time_us = esp_timer_get_time();

    // // Convert to seconds
    // double time_s = time_us / 1000000.0;

    // ESP_LOGI("TIME_SINCE_BOOT", "Time since boot: %.6f seconds", time_s);

    // int64_t t_start = esp_timer_get_time();
    if (redraw(decode_redraw(*buffer)) != 0) {
      return -1;
    }
    // int64_t t_end = esp_timer_get_time();
    // int64_t elapsed_us = t_end - t_start;
    // ESP_LOGI(TAG, "redraw completed in %lld µs", elapsed_us);
    break;
  }
  case OP_SET_CONFIG: {
    if (set_config(decode_set_config(*buffer)) != 0) {
      return -1;
    }
    break;
  }
  default:
    ESP_LOGW(TAG, "Unknown OpCode: 0x%02X", op_code);
    break;
  }

  return 0;
}
