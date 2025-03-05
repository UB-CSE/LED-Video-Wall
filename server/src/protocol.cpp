#include "protocol.hpp"
#include <cstdlib>
#include <cstring>

static uint8_t *allocate_message_buffer(uint32_t size) {
  return (uint8_t *)malloc(size);
}

uint32_t get_message_size(const uint8_t *buffer) {
  uint32_t size;
  memcpy(&size, buffer, sizeof(uint32_t));
  return size;
}

uint16_t get_message_op_code(const uint8_t *buffer) {
  uint16_t op_code;
  memcpy(&op_code, buffer + sizeof(uint32_t), sizeof(uint16_t));
  return op_code;
}

void free_message_buffer(void *buffer) { free(buffer); }

uint8_t *encode_set_leds(uint8_t gpio_pin, uint8_t bit_depth,
                         const uint8_t *pixel_data, uint32_t data_size,
                         uint32_t *out_size) {

  *out_size = sizeof(SetLedsMessage) + data_size;
  uint8_t *buffer = allocate_message_buffer(*out_size);
  if (!buffer)
    return NULL;

  SetLedsMessage *msg = (SetLedsMessage *)buffer;
  msg->header.size = *out_size;
  msg->header.op_code = OP_SET_LEDS;
  msg->gpio_pin = gpio_pin;
  msg->bit_depth = bit_depth;

  if (pixel_data && data_size > 0) {
    memcpy(msg->pixel_data, pixel_data, data_size);
  }

  return buffer;
}

uint8_t *encode_get_status(const char *debug_string, uint32_t *out_size) {
  uint32_t debug_len = debug_string ? strlen(debug_string) + 1 : 0;
  *out_size = sizeof(MessageHeader) + debug_len;

  uint8_t *buffer = allocate_message_buffer(*out_size);
  if (!buffer)
    return NULL;

  MessageHeader *header = (MessageHeader *)buffer;
  header->size = *out_size;
  header->op_code = OP_GET_STATUS;

  if (debug_string && debug_len > 0) {
    memcpy(buffer + sizeof(MessageHeader), debug_string, debug_len);
  }

  return buffer;
}

uint8_t *encode_set_brightness(uint16_t brightness, uint32_t *out_size) {
  *out_size = sizeof(SetBrightnessMessage);
  uint8_t *buffer = allocate_message_buffer(*out_size);
  if (!buffer)
    return NULL;

  SetBrightnessMessage *msg = (SetBrightnessMessage *)buffer;
  msg->header.size = *out_size;
  msg->header.op_code = OP_SET_BRIGHTNESS;
  msg->brightness = brightness;

  return buffer;
}

uint8_t *encode_redraw(uint32_t *out_size) {
  *out_size = sizeof(RedrawMessage);
  uint8_t *buffer = allocate_message_buffer(*out_size);
  if (!buffer)
    return NULL;

  RedrawMessage *msg = (RedrawMessage *)buffer;
  msg->header.size = *out_size;
  msg->header.op_code = OP_REDRAW;

  return buffer;
}

uint8_t *encode_set_config(uint8_t num_color_channels, uint16_t init_brightness,
                           uint8_t pins_used, const PinInfo *pin_info,
                           uint32_t *out_size) {

  *out_size = sizeof(SetConfigMessage) + (pins_used * sizeof(PinInfo));
  uint8_t *buffer = allocate_message_buffer(*out_size);
  if (!buffer)
    return NULL;

  SetConfigMessage *msg = (SetConfigMessage *)buffer;
  msg->header.size = *out_size;
  msg->header.op_code = OP_SET_CONFIG;
  msg->num_color_channels = num_color_channels;
  msg->init_brightness = init_brightness;
  msg->pins_used = pins_used;

  if (pin_info && pins_used > 0) {
    memcpy(msg->pin_info, pin_info, pins_used * sizeof(PinInfo));
  }

  return buffer;
}

uint8_t *encode_check_in(const uint8_t *mac_address, uint32_t *out_size) {
  *out_size = sizeof(CheckInMessage);
  uint8_t *buffer = allocate_message_buffer(*out_size);
  if (!buffer)
    return NULL;

  CheckInMessage *msg = (CheckInMessage *)buffer;
  msg->header.size = *out_size;
  msg->header.op_code = OP_CHECK_IN;

  if (mac_address) {
    memcpy(msg->mac_address, mac_address, 6);
  }

  return buffer;
}

uint8_t *encode_send_status(const char *debug_string, uint32_t *out_size) {
  uint32_t debug_len = debug_string ? strlen(debug_string) + 1 : 0;
  *out_size = sizeof(MessageHeader) + debug_len;

  uint8_t *buffer = allocate_message_buffer(*out_size);
  if (!buffer)
    return NULL;

  MessageHeader *header = (MessageHeader *)buffer;
  header->size = *out_size;
  header->op_code = OP_SEND_STATUS;

  if (debug_string && debug_len > 0) {
    memcpy(buffer + sizeof(MessageHeader), debug_string, debug_len);
  }

  return buffer;
}

SetLedsMessage *decode_set_leds(const uint8_t *buffer) {
  if (!buffer)
    return NULL;

  uint32_t message_size = get_message_size(buffer);
  if (message_size < sizeof(SetLedsMessage))
    return NULL;

  SetLedsMessage *msg = (SetLedsMessage *)malloc(message_size);
  if (!msg)
    return NULL;

  memcpy(msg, buffer, message_size);
  return msg;
}

GetStatusMessage *decode_get_status(const uint8_t *buffer) {
  if (!buffer)
    return NULL;

  uint32_t message_size = get_message_size(buffer);
  if (message_size < sizeof(MessageHeader))
    return NULL;

  GetStatusMessage *msg = (GetStatusMessage *)malloc(message_size);
  if (!msg)
    return NULL;

  memcpy(msg, buffer, message_size);
  return msg;
}

SetBrightnessMessage *decode_set_brightness(const uint8_t *buffer) {
  if (!buffer)
    return NULL;

  uint32_t message_size = get_message_size(buffer);
  if (message_size < sizeof(SetBrightnessMessage))
    return NULL;

  SetBrightnessMessage *msg = (SetBrightnessMessage *)malloc(message_size);
  if (!msg)
    return NULL;

  memcpy(msg, buffer, message_size);
  return msg;
}

RedrawMessage *decode_redraw(const uint8_t *buffer) {
  if (!buffer)
    return NULL;

  uint32_t message_size = get_message_size(buffer);
  if (message_size < sizeof(RedrawMessage))
    return NULL;

  RedrawMessage *msg = (RedrawMessage *)malloc(message_size);
  if (!msg)
    return NULL;

  memcpy(msg, buffer, message_size);
  return msg;
}

SetConfigMessage *decode_set_config(const uint8_t *buffer) {
  if (!buffer)
    return NULL;

  uint32_t message_size = get_message_size(buffer);
  if (message_size < sizeof(SetConfigMessage))
    return NULL;

  SetConfigMessage *msg = (SetConfigMessage *)malloc(message_size);
  if (!msg)
    return NULL;

  memcpy(msg, buffer, message_size);
  return msg;
}

CheckInMessage *decode_check_in(const uint8_t *buffer) {
  if (!buffer)
    return NULL;

  uint32_t message_size = get_message_size(buffer);
  if (message_size < sizeof(CheckInMessage))
    return NULL;

  CheckInMessage *msg = (CheckInMessage *)malloc(message_size);
  if (!msg)
    return NULL;

  memcpy(msg, buffer, message_size);
  return msg;
}

SendStatusMessage *decode_send_status(const uint8_t *buffer) {
  if (!buffer)
    return NULL;

  uint32_t message_size = get_message_size(buffer);
  if (message_size < sizeof(MessageHeader))
    return NULL;

  SendStatusMessage *msg = (SendStatusMessage *)malloc(message_size);
  if (!msg)
    return NULL;

  memcpy(msg, buffer, message_size);
  return msg;
}
