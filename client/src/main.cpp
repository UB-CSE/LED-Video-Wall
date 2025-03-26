#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "network.hpp"
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  connect_wifi();
}

void loop() {
  if (!socket.connected()) {
    Serial.println("Reconnecting to server...");
    send_checkin();
    vTaskDelay(pdMS_TO_TICKS(CHECK_IN_DELAY_MS));
    return;
  }

  if (socket.available()) {
    parse_tcp_message();
  }

  // To prevent the task watchdog timer.
  vTaskDelay(pdMS_TO_TICKS(10));
}
