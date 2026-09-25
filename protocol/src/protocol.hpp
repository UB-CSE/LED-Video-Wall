#pragma once

#include <cstdint>
#include <cstring>
#include <span>
#include <vector>
#include <algorithm>
#include <string_view>

enum class OperationCode : uint8_t {
  UNKNOWN = 0,
  SET_LEDS = 1,
  GET_LOGS,
  REDRAW,
  SET_CONFIG,
  CHECK_IN,
  SEND_LOGS,
  SET_LEDS_BATCHED,
};

enum class LEDType : uint8_t {
  WS2811 = 1,
  HUB75,
};

enum class ImageEncoding : uint8_t {
  UNKNOWN = 0,
  RGB_24,
  RGB_21,
  RGB_18,
  RGB_15,
  RGB_12,
  RGB_9,
  RGB_6,
  RGB_3,

  YUV_444,
  YUV_422, // YUYV
  // YUV_420, // NV12
};

const char* encoding_to_string(ImageEncoding encoding);
ImageEncoding encoding_from_string(std::string_view encoding_string);

#pragma pack(push, 1)

//
// Message structs
//

struct MessageHeader {
  uint32_t size;
  OperationCode op_code;
};

// set_leds

struct SetLEDsMessageHeader {
  MessageHeader header;
  int8_t gpio_pin;
  uint32_t num_leds;
};

struct SetLEDsMessage {
  SetLEDsMessageHeader header;
  uint8_t pixel_data[];
};

// set_leds_batched

struct SetLEDsBatchedMessageHeader {
  MessageHeader header;
  uint8_t batch_count;
};

struct LEDsBatchEntryHeader {
  int8_t gpio_pin;
  uint32_t num_leds;
};

struct LEDsBatchEntry {
  LEDsBatchEntryHeader header;
  uint8_t pixel_data[];
};

struct SetLEDsBatchedMessage {
  SetLEDsBatchedMessageHeader header;
  LEDsBatchEntry entries[];
};

struct LEDsBatchEntryData {
  int8_t gpio_pin;
  uint32_t num_leds;
  uint8_t *pixel_data;
};

// get_logs

struct GetLogsMessage {
  MessageHeader header;
};

// redraw

struct RedrawMessage {
  MessageHeader header;
};

// set_config

struct PinInfo {
  int8_t pin_num;
  LEDType led_type;
  uint32_t max_leds;
};

struct SetConfigMessageHeader {
  MessageHeader header;
  uint8_t pins_used;
  ImageEncoding encoding;
};

struct SetConfigMessage {
  SetConfigMessageHeader header;
  PinInfo pin_info[];
};

// check_in

struct CheckInMessage {
  MessageHeader header;
  uint8_t mac_address[6];
};

// send_logs

struct SendLogsMessage {
  MessageHeader header;
  const char logs[];
};

#pragma pack(pop)

//
// Message encode & decode functions
//

/**
 * Encode a set_leds message to send an image to display on a LED matrix.
 *
 * @param gpio_pin Pin index of the display.
 * @param num_leds Number of pixels.
 * @param pixel_data Pixel color data. Should be encoded in RGB24.
 * @param encoding Pixel encoding to convert to.
 *
 * @return Message buffer
 */
std::vector<uint8_t> encode_set_leds(int8_t gpio_pin, uint32_t num_leds,
                                     const uint8_t *pixel_data,
                                     ImageEncoding encoding);

/**
 * Encode a set_leds_batched message to send multiple images to display on
 * multiple LED matrices.
 *
 * @param entries Pin info and pixel data for each display to control. Should be
 * encoded in RGB24.
 * @param encoding Pixel encoding to convert to.
 *
 * @return Message buffer
 */
std::vector<uint8_t>
encode_set_leds_batched(std::span<const LEDsBatchEntryData> entries,
                        ImageEncoding encoding);

std::vector<uint8_t> encode_get_logs();

std::vector<uint8_t> encode_redraw();

std::vector<uint8_t> encode_set_config(std::span<const PinInfo> pin_info, ImageEncoding encoding);

std::vector<uint8_t> encode_check_in(uint8_t* mac_address);
std::vector<uint8_t> encode_check_in(std::array<uint8_t, 6> mac_address);

std::vector<uint8_t> encode_send_logs(const char *logs);

uint32_t get_message_size(const uint8_t* buffer);

OperationCode get_message_op_code(const uint8_t* buffer);

/**
 * Decode an encoded message.
 *
 * Note that pixel data will remain encoded so check config and convert before
 * using!
 *
 * @tparam T The message type to decode.
 * @param buffer The encoded message buffer to decode.
 *
 * @return The decoded message, or nullptr if invalid.
 */
template <typename T> const T *decode(const uint8_t* buffer) {
  if (get_message_size(buffer) < sizeof(MessageHeader)) {
    return nullptr;
  }
  return reinterpret_cast<const T *>(buffer);
}

/**
 * Get the size in bytes of an image in a given encoding format.
 *
 * @param num_leds Number of pixels in the image.
 * @param encoding Image encoding format.
 *
 * @return Size in bytes.
 */
size_t get_encoded_image_size(uint32_t num_leds, ImageEncoding encoding);
uint8_t get_bits_per_pixel(ImageEncoding encoding);

/**
 * Convert an image to a different encoding.
 *
 * @param num_leds Number of pixels in the image.
 * @param src Source image pixel array.
 * @param src_encoding Encoding of source image.
 * @param dest Destination image pixel array. Must be allocated to the right
 *             size (see get_encoded_image_size()).
 * @param dest_encoding Encoding of the destination image.
 *
 * @return Size in bytes of the destination image.
 */
size_t convert_image_encoding(uint32_t num_leds, const uint8_t *src,
                              ImageEncoding src_encoding, uint8_t *dest,
                              ImageEncoding dest_encoding);

class Pixel {
  bool is_rgb = true;
  uint8_t channels[3] = {};

public:
  Pixel() = default;

  Pixel(const Pixel &other) : is_rgb(other.is_rgb) {
    memcpy(channels, other.channels, 3);
  }

  Pixel &operator=(const Pixel &other) {
    if (&other != this) {
      is_rgb = other.is_rgb;
      memcpy(channels, other.channels, 3);
    }
    return *this;
  }

  static Pixel RGB(uint8_t r, uint8_t g, uint8_t b) {
    Pixel pixel(true);
    pixel.channels[0] = r;
    pixel.channels[1] = g;
    pixel.channels[2] = b;
    return pixel;
  }

  static Pixel RGB(const uint8_t *p) { return FromBuffer(true, p); }

  uint8_t R() const { return channels[0]; }
  uint8_t G() const { return channels[1]; }
  uint8_t B() const { return channels[2]; }

  uint8_t Y() const { return channels[0]; }
  uint8_t U() const { return channels[1]; }
  uint8_t V() const { return channels[2]; }

  const uint8_t *getChannels() const { return channels; }

  bool isRGB() const { return is_rgb; }
  bool isYUV() const { return !isRGB(); }

  // Limited-range BT.601 YUV -> RGB
  Pixel toRGB() const {
    if (isRGB()) {
      return *this;
    }

    const int y = std::max(0, static_cast<int>(Y()) - 16);
    const int u = static_cast<int>(U()) - 128;
    const int v = static_cast<int>(V()) - 128;

    const uint8_t r = clamp_u8((298 * y + 409 * v + 128) >> 8);
    const uint8_t g = clamp_u8((298 * y - 100 * u - 208 * v + 128) >> 8);
    const uint8_t b = clamp_u8((298 * y + 516 * u + 128) >> 8);

    return RGB(r, g, b);
  }

  // RGB -> limited-range BT.601 YUV
  Pixel toYUV() const {
    if (isYUV()) {
      return *this;
    }

    const int r = R();
    const int g = G();
    const int b = B();

    const uint8_t y = clamp_u8((77 * r + 150 * g + 29 * b + 128) >> 8);
    const uint8_t u = clamp_u8((-43 * r - 85 * g + 128 * b + 32768 + 128) >> 8);
    const uint8_t v = clamp_u8((128 * r - 107 * g - 21 * b + 32768 + 128) >> 8);

    return YUV(y, u, v);
  }

  static Pixel YUV(uint8_t y, uint8_t u, uint8_t v) {
    Pixel pixel(false);
    pixel.channels[0] = y;
    pixel.channels[1] = u;
    pixel.channels[2] = v;
    return pixel;
  }

  static Pixel YUV(const uint8_t *p) { return FromBuffer(false, p); }

private:
  Pixel(bool is_rgb) : is_rgb(is_rgb) {}

  static Pixel FromBuffer(bool is_rgb, const uint8_t *p) {
    Pixel pixel(is_rgb);
    std::memcpy(pixel.channels, p, 3);
    return pixel;
  }

  static inline uint8_t clamp_u8(const int x) {
    return static_cast<uint8_t>(std::clamp(x, 0, 255));
  }
};

/**
 * Decode a pixel from a buffer.
 *
 * @param head Pointer to current byte in input buffer.
 * @param bit Bit offset within *head, (0-7).
 * @param pixel_index Index of the pixel being decoded.
 * @param encoding Encoding format.
 * @return The decoded pixel.
 */
Pixel decode_pixel(const uint8_t *&head, uint8_t &bit, uint32_t pixel_index,
                   ImageEncoding encoding);

/**
 * Encode a pixel into a buffer.
 *
 * @param pixel The pixel to encode.
 * @param encoding Encoding format.
 * @param pixel_index Index of the pixel being encoded.
 * @param head Pointer to the current byte in the output buffer.
 * @param bit Bit offset witin *head, (0-7).
 */
void encode_pixel(Pixel pixel, ImageEncoding encoding, uint32_t pixel_index,
                  uint8_t *&head, uint8_t &bit);
