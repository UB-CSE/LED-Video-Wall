#ifndef SET_CONFIG_H
#define SET_CONFIG_H

#include "led_strip.h"
#include "protocol.hpp"
#include <Arduino.h>
#include <map>

extern std::map<uint8_t, led_strip_handle_t> pin_to_handle;

void set_config(SetConfigMessage *msg);
void clear_led_strips();

#endif
