#ifndef LED_PROTOCOL_H
#define LED_PROTOCOL_H

#include <cstdint>
#define OP_SET_LEDS 0x01
#define OP_GET_LOGS 0x02
#define OP_REDRAW 0x03
#define OP_SET_CONFIG 0x04
#define OP_CHECK_IN 0x05
#define OP_SEND_LOGS 0x06

#define LED_TYPE_WS2811 0x01

#define COLOR_ORDER_GRB 0x01

#pragma pack(push, 1)

typedef struct {
  uint32_t size;
  uint8_t op_code;
} MessageHeader;

typedef struct {
  MessageHeader header;
  uint8_t gpio_pin;
  uint8_t pixel_data[];
} SetLedsMessage;

typedef struct {
  MessageHeader header;
} GetLogsMessage;

typedef struct {
  MessageHeader header;
} RedrawMessage;

typedef struct {
  uint8_t pin_num;
  uint8_t color_order;
  uint32_t max_leds;
  uint8_t led_type;
} PinInfo;

typedef struct {
  MessageHeader header;
  uint8_t num_color_channels;
  uint8_t pins_used;
  PinInfo pin_info[];
} SetConfigMessage;

typedef struct {
  MessageHeader header;
  uint8_t mac_address[6];
} CheckInMessage;

typedef struct {
  MessageHeader header;
} SendLogsMessage;

#pragma pack(pop)

uint8_t *encode_set_leds(uint8_t gpio_pin, const uint8_t *pixel_data,
                         uint32_t data_size, uint32_t *out_size);

uint8_t *encode_get_logs(const char *debug_string, uint32_t *out_size);

uint8_t *encode_redraw(uint32_t *out_size);

uint8_t *encode_set_config(uint8_t num_color_channels, uint8_t pins_used,
                           const PinInfo *pin_info, uint32_t *out_size);

uint8_t *encode_check_in(const uint8_t *mac_address, uint32_t *out_size);

uint8_t *encode_send_logs(const char *buffer, uint32_t *out_size);

uint8_t get_message_op_code(const uint8_t *buffer);

SetLedsMessage *decode_set_leds(const uint8_t *buffer);

GetLogsMessage *decode_get_logs(const uint8_t *buffer);

RedrawMessage *decode_redraw(const uint8_t *buffer);

SetConfigMessage *decode_set_config(const uint8_t *buffer);

CheckInMessage *decode_check_in(const uint8_t *buffer);

SendLogsMessage *decode_send_logs(const uint8_t *buffer);

uint32_t get_message_size(const uint8_t *buffer);
void free_message_buffer(void *buffer);

#endif
