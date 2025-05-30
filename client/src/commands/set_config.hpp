#ifndef SET_CONFIG_H
#define SET_CONFIG_H

#include <map>

#include "led_strip.h"
#include "protocol.hpp"

extern std::map<uint8_t, led_strip_handle_t> pin_to_handle;
extern SemaphoreHandle_t pin_to_handle_mutex;

int set_config(SetConfigMessage *msg);

#endif
