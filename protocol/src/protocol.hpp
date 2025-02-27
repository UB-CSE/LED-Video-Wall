#ifndef LED_PROTOCOL_H
#define LED_PROTOCOL_H

#include <cstdint>
#define OP_SET_LEDS 0x01
#define OP_GET_STATUS 0x02
#define OP_SET_BRIGHTNESS 0x03
#define OP_REDRAW 0x04
#define OP_SET_CONFIG 0x05
#define OP_CHECK_IN 0x06
#define OP_SEND_STATUS 0x07

#define LED_TYPE_WS2811 0x01

#define COLOR_ORDER_GRB 0x01

#pragma pack(push, 1)

typedef struct {
  uint32_t size;
  uint16_t op_code;
} MessageHeader;

typedef struct {
  MessageHeader header;
  uint8_t gpio_pin;
  uint8_t bit_depth;
  uint8_t pixel_data[];
} SetLedsMessage;

typedef struct {
  MessageHeader header;
  char debug_string[];
} GetStatusMessage;

typedef struct {
  MessageHeader header;
  uint16_t brightness;
} SetBrightnessMessage;

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
  uint16_t init_brightness;
  uint8_t pins_used;
  PinInfo pin_info[];
} SetConfigMessage;

typedef struct {
  MessageHeader header;
  uint8_t mac_address[6];
} CheckInMessage;

typedef struct {
  MessageHeader header;
  char debug_string[];
} SendStatusMessage;

#pragma pack(pop)

uint8_t *encode_set_leds(uint8_t gpio_pin, uint8_t bit_depth,
                         const uint8_t *pixel_data, uint32_t data_size,
                         uint32_t *out_size);

uint8_t *encode_get_status(const char *debug_string, uint32_t *out_size);

uint8_t *encode_set_brightness(uint16_t brightness, uint32_t *out_size);

uint8_t *encode_redraw(uint32_t *out_size);

uint8_t *encode_set_config(uint8_t num_color_channels, uint16_t init_brightness,
                           uint8_t pins_used, const PinInfo *pin_info,
                           uint32_t *out_size);

uint8_t *encode_check_in(const uint8_t *mac_address, uint32_t *out_size);

uint8_t *encode_send_status(const char *debug_string, uint32_t *out_size);

uint16_t get_message_op_code(const uint8_t *buffer);

SetLedsMessage *decode_set_leds(const uint8_t *buffer);

GetStatusMessage *decode_get_status(const uint8_t *buffer);

SetBrightnessMessage *decode_set_brightness(const uint8_t *buffer);

RedrawMessage *decode_redraw(const uint8_t *buffer);

SetConfigMessage *decode_set_config(const uint8_t *buffer);

CheckInMessage *decode_check_in(const uint8_t *buffer);

SendStatusMessage *decode_send_status(const uint8_t *buffer);

uint32_t get_message_size(const uint8_t *buffer);
void free_message_buffer(void *buffer);

#endif
