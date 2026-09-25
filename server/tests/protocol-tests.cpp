#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <opencv2/opencv.hpp>
#include <protocol.hpp>

using namespace testing;
using enum ImageEncoding;

static const std::filesystem::path ImagesDir(IMAGES_DIR);
static const std::filesystem::path ParrotTestImagePath =
    ImagesDir / "parrot.jpg";
static constexpr int ParrotTestImageWidth = 150;
static constexpr int ParrotTestImageHeight = 200;

static const std::filesystem::path ButterflyTestImagePath =
    ImagesDir / "butterfly.jpg";
static constexpr int ButterflyTestImageWidth = 256;
static constexpr int ButterflyTestImageHeight = 256;

static void loadTestImage(cv::Mat &image,
                          const std::filesystem::path &imagePath, int width,
                          int height) {
  image = cv::imread(imagePath.string(), cv::IMREAD_COLOR);
  cv::cvtColor(image, image, cv::COLOR_BGR2RGB);

  // Sanity check
  ASSERT_THAT(image.size().width, Eq(width));
  ASSERT_THAT(image.size().height, Eq(height));
  ASSERT_THAT(image.total(), Eq(width * height));
  ASSERT_THAT(image.elemSize1(), Eq(1)); // 1 byte per channel
  ASSERT_THAT(image.elemSize(), Eq(3));  // 3 channels
}

TEST(Protocol, SetLEDs) {
  cv::Mat image;
  loadTestImage(image, ParrotTestImagePath, ParrotTestImageWidth,
                ParrotTestImageHeight);

  // Encode

  const size_t numLEDs = image.total();

  std::vector<uint8_t> buffer =
      encode_set_leds(-1, numLEDs, image.data, RGB_24);

  const size_t imageSize = image.total() * image.elemSize();
  const size_t expectedMsgSize = sizeof(SetLEDsMessageHeader) + imageSize;

  ASSERT_THAT(buffer, SizeIs(expectedMsgSize));

  // Decode

  auto *msg = decode<SetLEDsMessage>(buffer.data());
  ASSERT_THAT(msg, NotNull());

  ASSERT_THAT(msg->header.header.size, Eq(expectedMsgSize));
  ASSERT_THAT(msg->header.header.op_code, Eq(OperationCode::SET_LEDS));
  ASSERT_THAT(msg->header.gpio_pin, Eq(-1));
  ASSERT_THAT(msg->header.num_leds, Eq(numLEDs));

  // No conversion
  ASSERT_THAT(0 == std::memcmp(msg->pixel_data, image.data, imageSize),
              IsTrue());
}

TEST(Protocol, SetLEDsBatched) {
  cv::Mat parrotImage, butterflyImage;
  loadTestImage(parrotImage, ParrotTestImagePath, ParrotTestImageWidth,
                ParrotTestImageHeight);
  loadTestImage(butterflyImage, ButterflyTestImagePath, ButterflyTestImageWidth,
                ButterflyTestImageHeight);

  constexpr size_t numImages = 2;

  // Encode

  std::vector<LEDsBatchEntryData> entries(numImages);
  entries[0].gpio_pin = -1;
  entries[0].num_leds = parrotImage.total();
  entries[0].pixel_data = parrotImage.data;

  entries[1].gpio_pin = -2;
  entries[1].num_leds = butterflyImage.total();
  entries[1].pixel_data = butterflyImage.data;

  std::vector<uint8_t> buffer = encode_set_leds_batched(entries, RGB_24);

  const size_t parrotImageSize = parrotImage.total() * parrotImage.elemSize();
  const size_t butterflyImageSize =
      butterflyImage.total() * butterflyImage.elemSize();

  const size_t expectedMsgSize = sizeof(SetLEDsBatchedMessageHeader) +
                                 2 * sizeof(LEDsBatchEntryHeader) +
                                 parrotImageSize + butterflyImageSize;

  ASSERT_THAT(buffer, SizeIs(expectedMsgSize));

  // Decode

  auto *msg = decode<SetLEDsBatchedMessage>(buffer.data());
  ASSERT_THAT(msg, NotNull());

  ASSERT_THAT(msg->header.header.size, Eq(expectedMsgSize));
  ASSERT_THAT(msg->header.header.op_code, Eq(OperationCode::SET_LEDS_BATCHED));
  ASSERT_THAT(msg->header.batch_count, Eq(numImages));

  const auto *p = reinterpret_cast<const uint8_t *>(msg->entries);

  const auto *parrotEntry = reinterpret_cast<const LEDsBatchEntry *>(p);
  ASSERT_THAT(parrotEntry->header.gpio_pin, Eq(-1));
  ASSERT_THAT(parrotEntry->header.num_leds, Eq(parrotImage.total()));
  // No conversion
  ASSERT_THAT(0 == std::memcmp(parrotEntry->pixel_data, parrotImage.data,
                               parrotImageSize),
              IsTrue());

  p += sizeof(LEDsBatchEntryHeader) + parrotImageSize;

  const auto *butterflyEntry = reinterpret_cast<const LEDsBatchEntry *>(p);
  ASSERT_THAT(butterflyEntry->header.gpio_pin, Eq(-2));
  ASSERT_THAT(butterflyEntry->header.num_leds, Eq(butterflyImage.total()));
  // No conversion
  ASSERT_THAT(0 == std::memcmp(butterflyEntry->pixel_data, butterflyImage.data,
                               butterflyImageSize),
              IsTrue());
}

TEST(Protocol, SetConfig) {
  // Encode

  std::vector<PinInfo> pinInfo(2);
  pinInfo[0].pin_num = -1;
  pinInfo[0].max_leds = 4096;
  pinInfo[0].led_type = LEDType::HUB75;

  pinInfo[1].pin_num = 3;
  pinInfo[1].max_leds = 1024;
  pinInfo[1].led_type = LEDType::WS2811;

  std::vector<uint8_t> buffer = encode_set_config(pinInfo, RGB_12);

  constexpr size_t expectedMsgSize =
      sizeof(SetConfigMessageHeader) + 2 * sizeof(PinInfo);
  ASSERT_THAT(buffer, SizeIs(expectedMsgSize));

  // Decode

  auto *msg = decode<SetConfigMessage>(buffer.data());
  ASSERT_THAT(msg, NotNull());

  ASSERT_THAT(msg->header.header.size, Eq(expectedMsgSize));
  ASSERT_THAT(msg->header.header.op_code, Eq(OperationCode::SET_CONFIG));
  ASSERT_THAT(msg->header.pins_used, Eq(2));
  ASSERT_THAT(msg->header.encoding, Eq(RGB_12));

  ASSERT_THAT(msg->pin_info[0].pin_num, Eq(-1));
  ASSERT_THAT(msg->pin_info[0].max_leds, Eq(4096));
  ASSERT_THAT(msg->pin_info[0].led_type, Eq(LEDType::HUB75));

  ASSERT_THAT(msg->pin_info[1].pin_num, Eq(3));
  ASSERT_THAT(msg->pin_info[1].max_leds, Eq(1024));
  ASSERT_THAT(msg->pin_info[1].led_type, Eq(LEDType::WS2811));
}

TEST(Protocol, Redraw) {
  // Encode

  std::vector<uint8_t> buffer = encode_redraw();

  constexpr size_t expectedMsgSize = sizeof(RedrawMessage);
  ASSERT_THAT(buffer, SizeIs(expectedMsgSize));

  // Decode

  auto *msg = decode<RedrawMessage>(buffer.data());
  ASSERT_THAT(msg, NotNull());

  ASSERT_THAT(msg->header.size, Eq(expectedMsgSize));
  ASSERT_THAT(msg->header.op_code, Eq(OperationCode::REDRAW));
}

TEST(Protocol, CheckIn) {
  // Encode

  const std::array<uint8_t, 6> macAddr = {0xAA, 0xBB, 0xCC, 0xEE, 0xFF, 0xAA};

  std::vector<uint8_t> buffer = encode_check_in(macAddr);

  constexpr size_t expectedMsgSize = sizeof(CheckInMessage);
  ASSERT_THAT(buffer, SizeIs(expectedMsgSize));

  // Decode

  auto *msg = decode<CheckInMessage>(buffer.data());
  ASSERT_THAT(msg, NotNull());

  ASSERT_THAT(msg->header.size, Eq(expectedMsgSize));
  ASSERT_THAT(msg->header.op_code, Eq(OperationCode::CHECK_IN));

  ASSERT_THAT(msg->mac_address, ElementsAreArray(macAddr));
}

TEST(Protocol, GetLogs) {
  // Encode

  std::vector<uint8_t> buffer = encode_get_logs();

  constexpr size_t expectedMsgSize = sizeof(GetLogsMessage);
  ASSERT_THAT(buffer, SizeIs(expectedMsgSize));

  // Decode

  auto *msg = decode<GetLogsMessage>(buffer.data());
  ASSERT_THAT(msg, NotNull());

  ASSERT_THAT(msg->header.size, Eq(expectedMsgSize));
  ASSERT_THAT(msg->header.op_code, Eq(OperationCode::GET_LOGS));
}

TEST(Protocol, SendLogs) {
  // Encode

  const char *logs = "my log message";

  std::vector<uint8_t> buffer = encode_send_logs(logs);

  const size_t expectedMsgSize = sizeof(MessageHeader) + strlen(logs) + 1;
  ASSERT_THAT(buffer, SizeIs(expectedMsgSize));

  // Decode

  auto *msg = decode<SendLogsMessage>(buffer.data());
  ASSERT_THAT(msg, NotNull());

  ASSERT_THAT(msg->header.size, Eq(expectedMsgSize));
  ASSERT_THAT(msg->header.op_code, Eq(OperationCode::SEND_LOGS));

  ASSERT_THAT(msg->logs, StrEq(logs));
}

TEST(Protocol, GetRawPixel) {}

TEST(ImageEncoding, IdentityConversionRGB24) {
  const std::vector<uint8_t> src = {
      0xFF, 0x00, 0xAA, // Pixel 0: R=255, G=0, B=170
      0x12, 0x34, 0x56  // Pixel 1: R=18,  G=52, B=86
  };
  std::vector<uint8_t> dest(src.size(), 0x00);

  const size_t bytes_written =
      convert_image_encoding(2, src.data(), RGB_24, dest.data(), RGB_24);

  ASSERT_THAT(bytes_written, Eq(src.size()));
  ASSERT_THAT(dest, ElementsAreArray(src));
}

TEST(ImageEncoding, ZeroLedsReturnsZeroBytes) {
  const std::vector<uint8_t> src = {0xFF, 0xFF, 0xFF};
  std::vector<uint8_t> dest(3, 0xAA);

  const size_t bytes_written =
      convert_image_encoding(0, src.data(), RGB_24, dest.data(), RGB_12);

  ASSERT_THAT(bytes_written, Eq(0u));
}

TEST(ImageEncoding, RGB24ToRGB12Packing) {
  /*
   Pixel 0 (24-bit): R=0xF0, G=0xA0, B=0x50 -> 12-bit: R=0xF, G=0xA, B=0x5
   Pixel 1 (24-bit): R=0xC0, G=0x80, B=0x30 -> 12-bit: R=0xC, G=0x8, B=0x3

   Packed bits (LSB first):
   Byte 0: [P0_G: 4b][P0_R: 4b] = 0xAF
   Byte 1: [P1_R: 4b][P0_B: 4b] = 0xC5
   Byte 2: [P1_B: 4b][P1_G: 4b] = 0x38
   */

  const std::vector<uint8_t> src = {0xF0, 0xA0, 0x50, 0xC0, 0x80, 0x30};

  constexpr size_t expected_size = 3;
  std::vector<uint8_t> dest(expected_size, 0x00);

  const size_t bytes_written =
      convert_image_encoding(2, src.data(), RGB_24, dest.data(), RGB_12);

  ASSERT_THAT(bytes_written, Eq(expected_size));
  ASSERT_THAT(dest, ElementsAre(0xAF, 0xC5, 0x38));
}

TEST(ImageEncoding, RGB12ToRGB24Unpacking) {
  const std::vector<uint8_t> src = {0xAF, 0xC5, 0x38};

  constexpr size_t expected_size = 6;
  std::vector<uint8_t> dest(expected_size, 0x00);

  const size_t bytes_written =
      convert_image_encoding(2, src.data(), RGB_12, dest.data(), RGB_24);

  ASSERT_THAT(bytes_written, Eq(expected_size));
  ASSERT_THAT(dest, ElementsAre(0xF0, 0xA0, 0x50, 0xC0, 0x80, 0x30));
}

TEST(ImageEncoding, RGBConversionGamut) {
  const std::vector<uint8_t> src = {0xF1, 0xA2, 0x53, 0xC4, 0x85, 0x36};
  constexpr uint32_t num_leds = 2;

  for (int encoding_int = static_cast<int>(RGB_24);
       encoding_int <= static_cast<int>(RGB_3); encoding_int++) {
    const auto encoding = static_cast<ImageEncoding>(encoding_int);

    const uint8_t bits_per_pixel = get_bits_per_pixel(encoding);

    const size_t expected_size = get_encoded_image_size(num_leds, encoding);
    std::vector<uint8_t> conv(expected_size, 0x00);

    size_t bytes_written = convert_image_encoding(num_leds, src.data(), RGB_24,
                                                  conv.data(), encoding);

    ASSERT_THAT(bytes_written, Eq(expected_size));

    std::vector<uint8_t> final(6, 0x00);

    bytes_written = convert_image_encoding(num_leds, conv.data(), encoding,
                                           final.data(), RGB_24);
    ASSERT_THAT(bytes_written, Eq(6));

    std::vector<uint8_t> expected_result = src;
    for (uint8_t &v : expected_result) {
      const uint8_t channel_bits = bits_per_pixel / 3;
      const uint8_t offset = 8 - channel_bits;

      v >>= offset;
      v <<= offset;
    }

    ASSERT_THAT(final, ElementsAreArray(expected_result));
  }
}

TEST(ImageEncoding, YUV_444) {
  const std::vector<uint8_t> src = {241, 162, 83, 196, 133, 54};
  constexpr uint32_t num_leds = 2;

  const size_t expected_size = get_encoded_image_size(num_leds, YUV_444);
  ASSERT_THAT(expected_size, Eq(6));

  std::vector<uint8_t> conv(expected_size, 0x00);

  size_t bytes_written = convert_image_encoding(num_leds, src.data(), RGB_24,
                                                conv.data(), YUV_444);

  ASSERT_THAT(bytes_written, Eq(expected_size));
  ASSERT_THAT(conv, ElementsAre(0xB1, 0x4B, 0xAE, 0x8F, 0x4E, 0xA6));

  std::vector<uint8_t> final(6, 0x00);

  bytes_written = convert_image_encoding(num_leds, conv.data(), YUV_444,
                                         final.data(), RGB_24);
  ASSERT_THAT(bytes_written, Eq(6));

  ASSERT_THAT(final, ElementsAre(255, 171, 81, 209, 136, 47));
}

TEST(ImageEncoding, YUV_422) {
  const std::vector<uint8_t> src = {241, 162, 83, 196, 133, 54};
  constexpr uint32_t num_leds = 2;

  const size_t expected_size = get_encoded_image_size(num_leds, YUV_422);
  ASSERT_THAT(expected_size, Eq(4));

  std::vector<uint8_t> conv(expected_size, 0x00);

  size_t bytes_written = convert_image_encoding(num_leds, src.data(), RGB_24,
                                                conv.data(), YUV_422);

  ASSERT_THAT(bytes_written, Eq(expected_size));
  ASSERT_THAT(conv, ElementsAre(0xB1, 0x4D, 0x8F, 0xAA));

  std::vector<uint8_t> final(6, 0x00);

  bytes_written = convert_image_encoding(num_leds, conv.data(), YUV_422,
                                         final.data(), RGB_24);
  ASSERT_THAT(bytes_written, Eq(6));

  ASSERT_THAT(final, ElementsAre(255, 173, 85, 215, 134, 45));
}



