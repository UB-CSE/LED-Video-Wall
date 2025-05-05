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

uint8_t get_message_op_code(const uint8_t *buffer) {
  uint8_t op_code;
  memcpy(&op_code, buffer + sizeof(uint32_t), sizeof(uint8_t));
  return op_code;
}

void free_message_buffer(void *buffer) { free(buffer); }

uint8_t *encode_set_leds(uint8_t gpio_pin, const uint8_t *pixel_data,
                         uint32_t data_size, uint32_t *out_size) {

  *out_size = sizeof(SetLedsMessage) + data_size;
  uint8_t *buffer = allocate_message_buffer(*out_size);
  if (!buffer)
    return NULL;

  SetLedsMessage *msg = (SetLedsMessage *)buffer;
  msg->header.size = *out_size;
  msg->header.op_code = OP_SET_LEDS;
  msg->gpio_pin = gpio_pin;

  if (pixel_data && data_size > 0) {
    memcpy(msg->pixel_data, pixel_data, data_size);
  }

  return buffer;
}

uint8_t *encode_set_leds_batched(uint8_t batch_count, const LedsBatch *batches,
                                 uint32_t *out_size) {
  uint32_t payload = 0;
  for (uint8_t i = 0; i < batch_count; ++i) {
    payload += sizeof(LedsBatchEntryHeader) + batches[i].num_leds * 3;
  }

  *out_size = sizeof(MessageHeader) + 1 + payload;
  uint8_t *buf = allocate_message_buffer(*out_size);
  if (!buf)
    return NULL;

  MessageHeader *mh = (MessageHeader *)buf;
  mh->size = *out_size;
  mh->op_code = OP_SET_LEDS_BATCHED;

  uint8_t *p = buf + sizeof(MessageHeader);
  *p++ = batch_count;

  for (uint8_t i = 0; i < batch_count; ++i) {
    const LedsBatch *b = &batches[i];
    LedsBatchEntryHeader *eh = (LedsBatchEntryHeader *)p;
    eh->gpio_pin = b->gpio_pin;
    eh->num_leds = b->num_leds;
    p += sizeof(*eh);
    memcpy(p, b->pixel_data, b->num_leds * 3);
    p += b->num_leds * 3;
  }

  return buf;
}

// Like encode_set_leds, but doesn't copy the pixel data for you; that is, only
// the fixed size parts of the message are set.
SetLedsMessage *encode_fixed_set_leds(uint8_t gpio_pin, uint32_t data_size,
                                      uint32_t *out_size) {
  *out_size = sizeof(SetLedsMessage) + data_size;
  uint8_t *buffer = allocate_message_buffer(*out_size);
  if (!buffer)
    return NULL;

  SetLedsMessage *msg = (SetLedsMessage *)buffer;
  msg->header.size = *out_size;
  msg->header.op_code = OP_SET_LEDS;
  msg->gpio_pin = gpio_pin;

  return msg;
}

uint8_t *encode_get_logs(uint32_t *out_size) {
  *out_size = sizeof(MessageHeader);

  uint8_t *buffer = allocate_message_buffer(*out_size);
  if (!buffer)
    return NULL;

  MessageHeader *header = (MessageHeader *)buffer;
  header->size = *out_size;
  header->op_code = OP_GET_LOGS;

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

uint8_t *encode_set_config(uint8_t num_color_channels, uint8_t pins_used,
                           const PinInfo *pin_info, uint32_t *out_size) {

  *out_size = sizeof(SetConfigMessage) + (pins_used * sizeof(PinInfo));
  uint8_t *buffer = allocate_message_buffer(*out_size);
  if (!buffer)
    return NULL;

  SetConfigMessage *msg = (SetConfigMessage *)buffer;
  msg->header.size = *out_size;
  msg->header.op_code = OP_SET_CONFIG;
  msg->num_color_channels = num_color_channels;
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

uint8_t *encode_send_logs(const char *logs, uint32_t *out_size) {
  uint32_t debug_len = logs ? strlen(logs) + 1 : 0;
  *out_size = sizeof(MessageHeader) + debug_len;

  uint8_t *buffer = allocate_message_buffer(*out_size);
  if (!buffer)
    return NULL;

  MessageHeader *header = (MessageHeader *)buffer;
  header->size = *out_size;
  header->op_code = OP_SEND_LOGS;

  if (logs && debug_len > 0) {
    memcpy(buffer + sizeof(MessageHeader), logs, debug_len);
  }

  return buffer;
}

SetLedsMessage *decode_set_leds(const uint8_t *buffer) {
  if (!buffer)
    return NULL;

  uint32_t message_size = get_message_size(buffer);
  if (message_size < sizeof(SetLedsMessage))
    return NULL;

  return (SetLedsMessage *)buffer;
}

SetLedsBatchedMessage *decode_set_leds_batched(const uint8_t *buffer) {
  if (!buffer)
    return NULL;
  uint32_t sz = get_message_size(buffer);
  if (sz < sizeof(MessageHeader) + 1)
    return NULL;
  return (SetLedsBatchedMessage *)buffer;
}

GetLogsMessage *decode_get_logs(const uint8_t *buffer) {
  if (!buffer)
    return NULL;

  uint32_t message_size = get_message_size(buffer);
  if (message_size < sizeof(MessageHeader))
    return NULL;

  return (GetLogsMessage *)buffer;
}

RedrawMessage *decode_redraw(const uint8_t *buffer) {
  if (!buffer)
    return NULL;

  uint32_t message_size = get_message_size(buffer);
  if (message_size < sizeof(RedrawMessage))
    return NULL;

  return (RedrawMessage *)buffer;
}

SetConfigMessage *decode_set_config(const uint8_t *buffer) {
  if (!buffer)
    return NULL;

  uint32_t message_size = get_message_size(buffer);
  if (message_size < sizeof(SetConfigMessage))
    return NULL;

  return (SetConfigMessage *)buffer;
}

CheckInMessage *decode_check_in(const uint8_t *buffer) {
  if (!buffer)
    return NULL;

  uint32_t message_size = get_message_size(buffer);
  if (message_size < sizeof(CheckInMessage))
    return NULL;

  return (CheckInMessage *)buffer;
}

SendLogsMessage *decode_send_logs(const uint8_t *buffer) {
  if (!buffer)
    return NULL;

  uint32_t message_size = get_message_size(buffer);
  if (message_size < sizeof(MessageHeader))
    return NULL;

  return (SendLogsMessage *)buffer;
}
