#include "protocol.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

const char *encoding_to_string(ImageEncoding encoding) {
  switch (encoding) {
    using enum ImageEncoding;
  case RGB_24:
    return "rgb24";
  case RGB_21:
    return "rgb21";
  case RGB_18:
    return "rgb18";
  case RGB_15:
    return "rgb15";
  case RGB_12:
    return "rgb12";
  case RGB_9:
    return "rgb9";
  case RGB_6:
    return "rgb6";
  case RGB_3:
    return "rgb3";
  case YUV_444:
    return "yuv444";
  case YUV_422:
    return "yuv422";
  default:
    return "unknown";
  }
}

ImageEncoding encoding_from_string(std::string_view encoding_string) {
  if (encoding_string == "rgb24") {
    return ImageEncoding::RGB_24;
  }
  if (encoding_string == "rgb21") {
    return ImageEncoding::RGB_21;
  }
  if (encoding_string == "rgb18") {
    return ImageEncoding::RGB_18;
  }
  if (encoding_string == "rgb15") {
    return ImageEncoding::RGB_15;
  }
  if (encoding_string == "rgb12") {
    return ImageEncoding::RGB_12;
  }
  if (encoding_string == "rgb9") {
    return ImageEncoding::RGB_9;
  }
  if (encoding_string == "rgb6") {
    return ImageEncoding::RGB_6;
  }
  if (encoding_string == "rgb3") {
    return ImageEncoding::RGB_3;
  }
  if (encoding_string == "yuv444") {
    return ImageEncoding::YUV_444;
  }
  if (encoding_string == "yuv422") {
    return ImageEncoding::YUV_422;
  }
  return ImageEncoding::UNKNOWN;
}

uint8_t get_bits_per_pixel(ImageEncoding encoding) {
  switch (encoding) {
    using enum ImageEncoding;
  case RGB_24:
  case YUV_444:
    return 24;
  case RGB_21:
    return 21;
  case RGB_18:
    return 18;
  case RGB_15:
    return 15;
  case RGB_12:
    return 12;
  case RGB_9:
    return 9;
  case RGB_6:
    return 6;
  case RGB_3:
    return 3;
  case YUV_422:
    return 16; // (4*8)/2
  case UNKNOWN:
    return 0;
  }
  return 0;
}

size_t get_encoded_image_size(uint32_t num_leds, ImageEncoding encoding) {
  const uint8_t bits_per_pixel = get_bits_per_pixel(encoding);
  const size_t total_bits = num_leds * bits_per_pixel;

  size_t bytes = total_bits / 8;
  if (total_bits % 8 != 0)
    bytes += 1;

  return bytes;
}

static Pixel read_packed_rgb_from_bits(uint64_t pixel_bits,
                                       uint8_t bits_per_pixel) {
  const uint8_t channel_bits = bits_per_pixel / 3;
  const uint64_t mask = UINT64_MAX >> (64 - channel_bits);

  const uint8_t offset = 8 - channel_bits; // Final channel values must be 0-255

  const uint8_t r = (pixel_bits & mask) << offset;
  pixel_bits >>= channel_bits;
  const uint8_t g = (pixel_bits & mask) << offset;
  pixel_bits >>= channel_bits;
  const uint8_t b = (pixel_bits & mask) << offset;

  Pixel pixel = Pixel::RGB(r, g, b);
  return pixel;
}

static uint64_t make_packed_rgb_bits(const Pixel &pixel,
                                     uint8_t bits_per_pixel) {
  const uint8_t channel_bits = bits_per_pixel / 3;

  const uint8_t offset = 8 - channel_bits;

  const uint64_t r = pixel.R() >> offset;
  const uint64_t g = pixel.G() >> offset;
  const uint64_t b = pixel.B() >> offset;

  const uint64_t pixel_bits =
      r | (g << channel_bits) | (b << (2 * channel_bits));
  return pixel_bits;
}

static uint64_t read_bits(const uint8_t *&p, uint8_t &bit, uint8_t num_bits) {
  // assert(bit < 8);
  // assert(num_bits <= 64);

  if (num_bits == 0)
    return 0;

  uint64_t value = 0;
  uint8_t bits_read = 0;

  while (bits_read < num_bits) {
    const uint8_t bits_left_in_byte = 8 - bit;
    const uint8_t bits_to_take = (num_bits - bits_read < bits_left_in_byte)
                                     ? (num_bits - bits_read)
                                     : bits_left_in_byte;

    const uint8_t mask = (1u << bits_to_take) - 1;
    const uint64_t chunk = (*p >> bit) & mask;

    value |= (chunk << bits_read);

    bit += bits_to_take;
    bits_read += bits_to_take;

    if (bit == 8) {
      bit = 0;
      ++p;
    }
  }
  return value;
}

static void write_bits(uint64_t value, uint8_t num_bits, uint8_t *&p,
                       uint8_t &bit) {
  // assert(bit < 8);
  // assert(num_bits <= 64);

  uint8_t bits_written = 0;

  while (bits_written < num_bits) {
    const uint8_t bits_left_in_byte = 8 - bit;
    const uint8_t bits_to_write = (num_bits - bits_written < bits_left_in_byte)
                                      ? (num_bits - bits_written)
                                      : bits_left_in_byte;

    const uint8_t mask = (1u << bits_to_write) - 1;
    const uint8_t chunk = (value >> bits_written) & mask;

    // Clear target window, then write bits
    *p = (*p & ~(mask << bit)) | (chunk << bit);

    bit += bits_to_write;
    bits_written += bits_to_write;

    if (bit == 8) {
      bit = 0;
      ++p;
    }
  }
}

static void encode_rgb(const Pixel &pixel, ImageEncoding encoding,
                       uint8_t *&head, uint8_t &bit) {
  const uint8_t bits_per_pixel = get_bits_per_pixel(encoding);
  const uint64_t bits = make_packed_rgb_bits(pixel, bits_per_pixel);
  write_bits(bits, bits_per_pixel, head, bit);
}

static Pixel decode_rgb(const uint8_t *&head, uint8_t &bit,
                        ImageEncoding encoding) {
  const uint8_t bits_per_pixel = get_bits_per_pixel(encoding);
  const uint64_t bits = read_bits(head, bit, bits_per_pixel);
  Pixel pixel = read_packed_rgb_from_bits(bits, get_bits_per_pixel(encoding));
  return pixel;
}

static void encode_yuyv(const Pixel &pixel, uint8_t *&head,
                        uint32_t pixel_index) {
  const bool even = pixel_index % 2 == 0;
  head[even ? 0 : 2] = pixel.Y();

  if (even) {
    // Save these for next iteration.
    head[1] = pixel.U();
    head[3] = pixel.V();
  }
  if (!even) {
    // Average chroma between two pixels.

    const uint8_t u0 = head[1];
    const uint8_t v0 = head[3];

    const auto u =
        static_cast<uint8_t>((static_cast<int>(pixel.U()) + u0 + 1) >> 1);

    const auto v =
        static_cast<uint8_t>((static_cast<int>(pixel.V()) + v0 + 1) >> 1);

    head[1] = u;
    head[3] = v;

    head += 4 * sizeof(uint8_t);
  }
}

static Pixel decode_yuyv(const uint8_t *&head, uint32_t pixel_index) {
  const uint8_t u = head[1];
  const uint8_t v = head[3];
  const bool even = pixel_index % 2 == 0;
  const uint8_t y = head[even ? 0 : 2];
  if (!even) {
    head += 4 * sizeof(uint8_t);
  }
  Pixel pixel = Pixel::YUV(y, u, v);
  return pixel;
}

#if false
static void encode_nv12(const Pixel &pixel, uint32_t pixel_index,
                        uint32_t image_width, uint32_t image_height,
                        uint8_t *&head, uint8_t *start) {
  *head = pixel.Y();
  head += sizeof(uint8_t);

  uint32_t row = pixel_index / image_width;
  uint32_t column = pixel_index % image_width;
  // split into 2x2 pixel "chunks"
  uint32_t chunk_index = (row / 2) + (column / 2);
  // Semi-planar: U & V for each chunk are after all pixel Y values
  uint8_t *uv_start = start + (image_width * image_height * sizeof(uint8_t));
  uint8_t *uv = uv_start + (chunk_index * 2 * sizeof(uint8_t));
  // TODO: Average chroma between 4 pixels
  uv[0] = pixel.U();
  uv[1] = pixel.V();
}

static Pixel decode_nv12(const uint8_t *&head, uint32_t pixel_index,
                         const uint8_t *start, uint32_t image_width,
                         uint32_t image_height) {
  uint8_t y = *head;
  head += sizeof(uint8_t);

  uint32_t row = pixel_index / image_width;
  uint32_t column = pixel_index % image_width;
  // split into 2x2 pixel "chunks"
  uint32_t chunk_index = (row / 2) + (column / 2);
  // Semi-planar: U & V for each chunk are after all pixel Y values
  const uint8_t *uv_start =
      start + (image_width * image_height * sizeof(uint8_t));
  const uint8_t *uv = uv_start + (chunk_index * 2 * sizeof(uint8_t));
  uint8_t u = uv[0];
  uint8_t v = uv[1];
  Pixel pixel = Pixel::YUV(y, u, v);
  return pixel;
}
#endif

Pixel decode_pixel(const uint8_t *&head, uint8_t &bit, uint32_t pixel_index,
                   ImageEncoding encoding) {
  using enum ImageEncoding;

  Pixel pixel{};
  switch (encoding) {
  case RGB_24:
    pixel = Pixel::RGB(head);
    head += 3 * sizeof(uint8_t);
    break;
  case RGB_21:
  case RGB_18:
  case RGB_15:
  case RGB_12:
  case RGB_9:
  case RGB_6:
  case RGB_3:
    pixel = decode_rgb(head, bit, encoding);
    break;
  case YUV_444:
    pixel = Pixel::YUV(head);
    head += 3 * sizeof(uint8_t);
    break;
  case YUV_422:
    pixel = decode_yuyv(head, pixel_index);
    break;
  case UNKNOWN:
    break;
  }

  return pixel;
}

void encode_pixel(Pixel pixel, ImageEncoding encoding, uint32_t pixel_index,
                  uint8_t *&head, uint8_t &bit) {
  using enum ImageEncoding;

  if (encoding >= YUV_444) {
    pixel = pixel.toYUV();
  } else {
    pixel = pixel.toRGB();
  }

  switch (encoding) {
  case RGB_24:
  case YUV_444:
    memcpy(head, pixel.getChannels(), 3 * sizeof(uint8_t));
    head += 3 * sizeof(uint8_t);
    break;
  case RGB_21:
  case RGB_18:
  case RGB_15:
  case RGB_12:
  case RGB_9:
  case RGB_6:
  case RGB_3:
    encode_rgb(pixel, encoding, head, bit);
    break;
  case YUV_422:
    encode_yuyv(pixel, head, pixel_index);
    break;
  case UNKNOWN:
    break;
  }
}

size_t convert_image_encoding(uint32_t num_leds, const uint8_t *src,
                              ImageEncoding src_encoding, uint8_t *dest,
                              ImageEncoding dest_encoding) {

  const size_t src_size = get_encoded_image_size(num_leds, src_encoding);
  const size_t dest_size = get_encoded_image_size(num_leds, dest_encoding);

  if (src_encoding == dest_encoding) {
    std::memcpy(dest, src, src_size);
    return src_size;
  }

  const uint8_t *src_head = src;
  uint8_t src_bit = 0;
  uint8_t *dest_head = dest;
  uint8_t dest_bit = 0;
  for (uint32_t pixel_index = 0; pixel_index < num_leds; pixel_index++) {
    const Pixel pixel =
        decode_pixel(src_head, src_bit, pixel_index, src_encoding);
    encode_pixel(pixel, dest_encoding, pixel_index, dest_head, dest_bit);
  }

  return dest_size;
}

std::vector<uint8_t> encode_set_leds(int8_t gpio_pin, uint32_t num_leds,
                                     const uint8_t *pixel_data,
                                     ImageEncoding encoding) {
  const size_t message_size =
      sizeof(SetLEDsMessage) + get_encoded_image_size(num_leds, encoding);
  std::vector<uint8_t> buffer(message_size);

  auto *msg = reinterpret_cast<SetLEDsMessageHeader *>(buffer.data());
  msg->header.size = message_size;
  msg->header.op_code = OperationCode::SET_LEDS;
  msg->gpio_pin = gpio_pin;
  msg->num_leds = num_leds;

  uint8_t *p = buffer.data() + sizeof(SetLEDsMessageHeader);

  if (pixel_data && num_leds > 0) {
    (void)convert_image_encoding(num_leds, pixel_data, ImageEncoding::RGB_24, p,
                                 encoding);
  }

  return buffer;
}

std::vector<uint8_t>
encode_set_leds_batched(std::span<const LEDsBatchEntryData> entries,
                        ImageEncoding encoding) {
  const uint8_t num_entries = std::min(entries.size(), size_t(UINT8_MAX));

  size_t payload_size = 0;

  for (uint8_t i = 0; i < num_entries; ++i) {
    size_t entry_size = sizeof(LEDsBatchEntryHeader) +
                        get_encoded_image_size(entries[i].num_leds, encoding);
    payload_size += entry_size;
  }

  const size_t message_size =
      sizeof(SetLEDsBatchedMessageHeader) + payload_size;
  std::vector<uint8_t> buf(message_size);

  // Header

  auto *mh = reinterpret_cast<SetLEDsBatchedMessageHeader *>(buf.data());
  mh->header.size = message_size;
  mh->header.op_code = OperationCode::SET_LEDS_BATCHED;
  mh->batch_count = num_entries;

  // Entries

  uint8_t *p = buf.data() + sizeof(SetLEDsBatchedMessageHeader);
  for (uint8_t i = 0; i < num_entries; ++i) {
    const LEDsBatchEntryData *e = &entries[i];
    auto *eh = reinterpret_cast<LEDsBatchEntryHeader *>(p);
    eh->gpio_pin = e->gpio_pin;
    eh->num_leds = e->num_leds;
    p += sizeof(LEDsBatchEntryHeader);

    const size_t image_size = convert_image_encoding(
        eh->num_leds, e->pixel_data, ImageEncoding::RGB_24, p, encoding);
    p += image_size;
  }

  return buf;
}

std::vector<uint8_t> encode_get_logs() {
  constexpr size_t message_size = sizeof(MessageHeader);

  std::vector<uint8_t> buffer(message_size);

  auto *header = reinterpret_cast<MessageHeader *>(buffer.data());
  header->size = message_size;
  header->op_code = OperationCode::GET_LOGS;

  return buffer;
}

std::vector<uint8_t> encode_redraw() {
  constexpr size_t message_size = sizeof(RedrawMessage);

  std::vector<uint8_t> buffer(message_size);

  auto *msg = reinterpret_cast<RedrawMessage *>(buffer.data());
  msg->header.size = message_size;
  msg->header.op_code = OperationCode::REDRAW;

  return buffer;
}

std::vector<uint8_t> encode_set_config(std::span<const PinInfo> pin_info,
                                       ImageEncoding encoding) {
  const uint8_t pins_used = std::min(pin_info.size(), size_t(UINT8_MAX));

  const size_t message_size =
      sizeof(SetConfigMessageHeader) + (pins_used * sizeof(PinInfo));
  std::vector<uint8_t> buffer(message_size);

  auto *msg = reinterpret_cast<SetConfigMessageHeader *>(buffer.data());
  msg->header.size = message_size;
  msg->header.op_code = OperationCode::SET_CONFIG;
  msg->pins_used = pins_used;
  msg->encoding = encoding;

  uint8_t *p = buffer.data() + sizeof(SetConfigMessageHeader);

  if (!pin_info.empty()) {
    memcpy(p, pin_info.data(), pins_used * sizeof(PinInfo));
  }

  return buffer;
}

std::vector<uint8_t> encode_check_in(uint8_t *mac_address) {
  constexpr size_t message_size = sizeof(CheckInMessage);
  std::vector<uint8_t> buffer(message_size);

  auto *msg = reinterpret_cast<CheckInMessage *>(buffer.data());
  msg->header.size = message_size;
  msg->header.op_code = OperationCode::CHECK_IN;

  memcpy(msg->mac_address, mac_address, 6);

  return buffer;
}

std::vector<uint8_t> encode_check_in(std::array<uint8_t, 6> mac_address) {
  return encode_check_in(mac_address.data());
}

std::vector<uint8_t> encode_send_logs(const char *logs) {
  const size_t debug_len = logs ? strlen(logs) + 1 : 0;
  const size_t message_size = sizeof(MessageHeader) + debug_len;

  std::vector<uint8_t> buffer(message_size);

  auto *header = reinterpret_cast<MessageHeader *>(buffer.data());
  header->size = message_size;
  header->op_code = OperationCode::SEND_LOGS;

  uint8_t *p = buffer.data() + sizeof(MessageHeader);

  if (logs && debug_len > 0) {
    memcpy(p, logs, debug_len);
  }

  return buffer;
}

uint32_t get_message_size(const uint8_t *buffer) {
  const auto *size_ptr = reinterpret_cast<const uint32_t *>(buffer);
  uint32_t size = *size_ptr;
  return size;
}

OperationCode get_message_op_code(const uint8_t *buffer) {
  if (get_message_size(buffer) < sizeof(MessageHeader)) {
    return OperationCode::UNKNOWN;
  }
  const auto *header = reinterpret_cast<const MessageHeader *>(buffer);
  return header->op_code;
}
