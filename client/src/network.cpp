#include "esp_log.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>  
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h> 

#include "commands/get_logs.hpp"
#include "commands/redraw.hpp"
#include "commands/set_config.hpp"
#include "commands/set_leds.hpp"
#include "network.hpp"
#include "protocol.hpp"

#include "esp_http_client.h"
#include "esp_crt_bundle.h"

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

#define DISCOVERY_URL_BASE "https://ledvwci.cse.buffalo.edu/client/get-server/"

//Buffer to hold full discovery URL
static char g_discovery_url[128];

//bigger buffer for host/IP
static char g_server_ip[64] = SERVER_IP;

struct HttpResponseBuffer {
  char *buf;
  int buf_size;
  int data_len;
};

static int build_discovery_url(void) {
  uint8_t mac[6];
  esp_err_t err = esp_wifi_get_mac(WIFI_IF_STA, mac);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to get MAC address: %s", esp_err_to_name(err));
    return -1;
  }

  int written = snprintf(
      g_discovery_url,
      sizeof(g_discovery_url),
      "%s%02x:%02x:%02x:%02x:%02x:%02x",
      DISCOVERY_URL_BASE,
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  if (written <= 0 || written >= (int)sizeof(g_discovery_url)) {
    ESP_LOGE(TAG, "Failed to build discovery URL (buffer too small?)");
    return -1;
  }

  ESP_LOGI(TAG, "Discovery URL: %s", g_discovery_url);
  return 0;
}

static esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
  if (evt->event_id == HTTP_EVENT_ON_DATA && evt->user_data) {
    auto *resp = static_cast<HttpResponseBuffer *>(evt->user_data);

    int copy_len = evt->data_len;
    if (resp->data_len + copy_len >= resp->buf_size) {
      copy_len = resp->buf_size - resp->data_len - 1; 
    }
    if (copy_len > 0) {
      memcpy(resp->buf + resp->data_len, evt->data, copy_len);
      resp->data_len += copy_len;
      resp->buf[resp->data_len] = '\0';
    }
  }
  return ESP_OK;
}

//Fetch server IP/host from HTTPS endpoint and store in g_server_ip.
static int update_server_ip_from_http(void) {
  if (build_discovery_url() != 0) {
    ESP_LOGE(TAG, "Could not build discovery URL");
    return -1;
  }

  ESP_LOGI(TAG, "Fetching server IP from discovery URL: %s", g_discovery_url);

  char body_buffer[64] = {0};
  HttpResponseBuffer resp{};
  resp.buf = body_buffer;
  resp.buf_size = sizeof(body_buffer);
  resp.data_len = 0;

  esp_http_client_config_t config = {};
  config.url = g_discovery_url;
  config.method = HTTP_METHOD_GET;
  config.event_handler = _http_event_handler;
  config.user_data = &resp;
  config.transport_type = HTTP_TRANSPORT_OVER_SSL;
  config.crt_bundle_attach = esp_crt_bundle_attach;

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

  int status = esp_http_client_get_status_code(client);
  int64_t content_length = esp_http_client_get_content_length(client);
  ESP_LOGI(TAG, "HTTP status=%d, content_length=%lld, body_len=%d",
           status, content_length, resp.data_len);

  esp_http_client_cleanup(client);

  if (resp.data_len <= 0) {
    ESP_LOGE(TAG, "Discovery response body is empty");
    return -1;
  }

  char *p = resp.buf;

  while (*p == '\r' || *p == '\n') {
    p++;
  }

  char *end = p + strlen(p);
  while (end > p && (end[-1] == '\r' || end[-1] == '\n')) {
    end--;
  }
  *end = '\0';

  if (*p == '\0') {
    ESP_LOGE(TAG, "Discovery response was CR/LF only");
    return -1;
  }

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

  if (update_server_ip_from_http() != 0) {
    ESP_LOGW(TAG, "Server discovery via HTTP failed, using default IP: %s",
             g_server_ip);
  }

  int sockfd = -1;
  struct sockaddr_in dest_addr;
  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.sin_family = AF_INET;

  if (inet_pton(AF_INET, g_server_ip, &dest_addr.sin_addr.s_addr) != 1) {
    ESP_LOGI(TAG, "Resolving hostname: %s", g_server_ip);
    struct addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    struct addrinfo *res = nullptr;

    int err = getaddrinfo(g_server_ip, nullptr, &hints, &res);
    if (err != 0 || res == nullptr) {
      ESP_LOGE(TAG, "DNS lookup failed for host '%s', err=%d", g_server_ip, err);
      if (res) {
        freeaddrinfo(res);
      }
      return -1;
    }

    struct sockaddr_in *addr_in = reinterpret_cast<struct sockaddr_in *>(res->ai_addr);
    dest_addr.sin_addr = addr_in->sin_addr;

    freeaddrinfo(res);
  }

  for (uint16_t port = SERVER_PORT_START; port <= SERVER_PORT_END; port++) {
    if (sockfd >= 0) {
      close(sockfd);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sockfd < 0) {
      ESP_LOGE(TAG, "Failed to create socket: %d", errno);
      continue;
    }

    struct timeval tv {
      RECV_TIMEOUT_SEC, 0
    };
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
      ESP_LOGE(TAG, "Failed to set socket recv timeout");
      close(sockfd);
      return -1;
    }

    dest_addr.sin_port = htons(port);

    if (connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) ==
        0) {
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
    return -1;
  } else if (message_size > 10000) {
    ESP_LOGE(TAG, "msg is way too big");
    return -1;
  }

  if (message_size > *buffer_size) {
    uint8_t *new_buffer = (uint8_t *)realloc(*buffer, message_size);
    if (new_buffer) {
      *buffer = new_buffer;
      *buffer_size = message_size;
    } else {
      ESP_LOGW(TAG, "Failed to resize message buffer");
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
    if (set_leds_batched(decode_set_leds_batched(*buffer)) != 0) {
      return -1;
    }
    break;
  }
  case OP_GET_LOGS: {
    if (get_logs(decode_get_logs(*buffer), sockfd) != 0) {
      return -1;
    }
    break;
  }
  case OP_REDRAW: {
    if (redraw(decode_redraw(*buffer)) != 0) {
      return -1;
    }
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
