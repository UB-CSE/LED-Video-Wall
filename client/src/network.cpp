#include "commands/get_status.hpp"
#include "commands/redraw.hpp"
#include "commands/set_config.hpp"
#include "commands/set_leds.hpp"
#include "esp_log.h"
#include "network.hpp"
#include "protocol.hpp"
#include <Arduino.h>
#include <WiFi.h>
#include <cstddef>
#include <cstdint>
#include <esp_wifi.h>

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

WiFiClient socket;

uint8_t *global_buffer = nullptr;
uint32_t global_buffer_size = 0;

// TODO: we may be able to auto set wifi in esp-idf settings
void connect_wifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  ESP_LOGI(TAG, "MAC Address: %s", WiFi.macAddress().c_str());
  ESP_LOGI(TAG, "Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(WIFI_RECONNECT_DELAY_MS));
    ESP_LOGD(TAG, ".");
  }

  ESP_LOGI(TAG, "Connected to WiFi");
  ESP_LOGI(TAG, "IP Address: %s", WiFi.localIP().toString().c_str());

  send_checkin();
}

void send_checkin() {
  ESP_LOGI(TAG, "Sending check-in message");

  // When attempting to connect to the server, there are multiple ports that it
  // may have connected to. The preprocessor constants SERVER_PORT_START and
  // SERVER_PORT_END denote the range of ports, start and end included, that the
  // server may be using. The code below attempts to connect on all the ports in
  // the range, and simply returns if none of the ports resulted in a successful
  // connection. If a connection does succeed then the loop ends early and
  // continues with the rest of the code.
  for (uint16_t port = SERVER_PORT_START; port <= SERVER_PORT_END; port++) {
    if (socket.connect(SERVER_IP, port)) {
      break;
    } else {
      ESP_LOGD(TAG,
               "Tried and failed to connect to server port: %hu; trying a "
               "different port",
               port);
      if (port == SERVER_PORT_END) {
        ESP_LOGI(TAG, "Failed to connect to any server port");
        return;
      }
    }
  }
  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);

  uint32_t msg_size = 0;
  uint8_t *buffer = encode_check_in(mac, &msg_size);
  if (buffer) {
    socket.write(buffer, msg_size);
    ESP_LOGI(TAG, "Check-in message sent");
    free_message_buffer(buffer);
  } else {
    ESP_LOGW(TAG, "Failed to encode check-in message");
  }
}

void parse_tcp_message() {
  uint8_t size_buffer[sizeof(uint32_t)];
  int bytes_read = socket.read(size_buffer, sizeof(size_buffer));
  if (bytes_read != sizeof(uint32_t)) {
    ESP_LOGW(TAG, "Failed to read message size");
    // TODO: it's possible to read only half the message size. if this were to
    // occur, then it's posssible that the next time parse_tcp_message is
    // called, the bytes are misaligned. we need another loop reading bytes in
    // this case
    return;
  }

  uint32_t message_size = get_message_size(size_buffer);

  ESP_LOGD(TAG, "Message size from header: %u bytes",
           (unsigned int)message_size);

  // The global buffer is resized to the maximum size message we can possibly
  // receive. Since we expect to receive that same size message multiple times,
  // this is a fair optimization.
  //
  // We may additionally consider resizing this buffer depending on the maximum
  // constraints given from a call to set_config, otherwise we'd expect this to
  // resize rather significantly on the first call to set_leds.
  //
  // TODO: if the client ever changes config, we can assume the size of the
  // global_buffer may decrease, so we should probably add some checks to calc
  // the max buffer size in set_config
  if (message_size > global_buffer_size) {
    uint8_t *new_buffer = (uint8_t *)realloc(global_buffer, message_size);
    if (new_buffer) {
      global_buffer = new_buffer;
      global_buffer_size = message_size;
    } else {
      ESP_LOGW(TAG, "Failed to resize message buffer");

      // When realloc fails, it deallocs the buffer and returns a null pointer,
      // so we must reset the size to its initial state.
      global_buffer_size = 0;

      // If we fail to realloc the buffer, then we can't read the whole message.
      // If we return from this function and it finds bytes available again, it
      // may start reading in the middle of a message, resulting in undefined
      // behavior. Thus, we close the socket and return, allowing the client to
      // reconnect and resume later.
      //
      // TODO: ideally we'd recognize how many bytes are left in the previous
      // message
      socket.stop();

      return;
    }
  }

  memcpy(global_buffer, size_buffer, sizeof(uint32_t));

  uint32_t remaining_bytes = message_size - sizeof(uint32_t);

  uint32_t total = 0;
  uint8_t *buffer_ptr = global_buffer + sizeof(uint32_t);
  while (total < remaining_bytes) {
    int current = socket.read(buffer_ptr + total, remaining_bytes - total);
    if (current <= 0) {
      if (!socket.connected()) {
        ESP_LOGW(TAG, "Socket disconnected");
        return;
      }

      ESP_LOGW(TAG, "Socket read failed");

      // Oftentimes socket.read will return 0 or -1, then after a few iterations
      // begin reading data again from the same message. The root of this issue
      // isn't clear, so we add a delay here to prevent hogging the CPU. Note
      // that 10ms is sufficient for the task watchdog to not trigger.
      vTaskDelay(pdMS_TO_TICKS(10));
    }

    total += current;
    ESP_LOGD(TAG, "Read %d bytes, total read: %u/%u bytes", current,
             (unsigned int)total, (unsigned int)remaining_bytes);
  }

  uint16_t op_code = get_message_op_code(global_buffer);
  ESP_LOGD(TAG, "Received OpCode: 0x%04X", op_code);

  switch (op_code) {
  case OP_SET_LEDS: {
    set_leds(decode_set_leds(global_buffer));
    break;
  }
  case OP_GET_STATUS: {
    get_status(decode_get_status(global_buffer));
    break;
  }
  case OP_REDRAW: {
    redraw(decode_redraw(global_buffer));
    break;
  }
  case OP_SET_CONFIG: {
    set_config(decode_set_config(global_buffer));
    break;
  }
  default:
    ESP_LOGW(TAG, "Unknown OpCode: 0x%02X", op_code);
    break;
  }
}
