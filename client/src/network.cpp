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

int read_exact(int sockfd, uint8_t *buffer, uint32_t len) {
  uint32_t total = 0;
  while (total < len) {
    ssize_t bytes_read = recv(sockfd, buffer + total, len - total, 0);
    if (bytes_read > 0) {
      total += bytes_read;
      ESP_LOGD(TAG, "Read %d bytes, total read: %u/%u bytes", bytes_read,
               (unsigned int)total, (unsigned int)len);
    } else if (bytes_read == 0) {
      ESP_LOGW(TAG, "Socket disconnected");
      return -1;
    } else {
      ESP_LOGW(TAG, "Error reading socket: %d", errno);
      return -1;
    }
  }

  return 0;
}

int checkin(int *out_sockfd) {
  ESP_LOGI(TAG, "Sending check-in message");

  int sockfd = -1;
  struct sockaddr_in dest_addr;
  dest_addr.sin_family = AF_INET;
  inet_pton(AF_INET, SERVER_IP, &dest_addr.sin_addr.s_addr);

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
      ESP_LOGI(TAG, "Connected to %s:%u", SERVER_IP, port);
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

  uint32_t msg_size = 0;
  uint8_t *buffer = encode_check_in(mac, &msg_size);
  if (!buffer) {
    ESP_LOGW(TAG, "Failed to encode check-in message");
    close(sockfd);
    return -1;
  }

  ssize_t written = send(sockfd, buffer, msg_size, 0);
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
  if (read_exact(sockfd, *buffer, sizeof(size_buffer)) != 0) {
    return -1;
  }

  uint32_t message_size = get_message_size(size_buffer);

  ESP_LOGD(TAG, "Message size from header: %u bytes",
           (unsigned int)message_size);

  // The buffer is resized to the maximum size message we can possibly
  // receive. Since we expect to receive that same size message multiple times,
  // this is a fair optimization.
  //
  // We may additionally consider resizing this buffer depending on the maximum
  // constraints given from a call to set_config, otherwise we'd expect this to
  // resize rather significantly on the first call to set_leds.
  if (message_size > *buffer_size) {
    uint8_t *new_buffer = (uint8_t *)realloc(buffer, message_size);
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
      close(sockfd);

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
