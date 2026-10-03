#include "tests-util.hpp"

#include <filesystem>
#include <opencv2/opencv.hpp>
#include <protocol.hpp>

using namespace testing;
using enum ImageEncoding;
using namespace std::string_literals;

static const std::filesystem::path ParrotTestImagePath =
    TestImagesDir / "parrot.jpg";
static constexpr int ParrotTestImageWidth = 150;
static constexpr int ParrotTestImageHeight = 200;

static const std::filesystem::path ButterflyTestImagePath =
    TestImagesDir / "butterfly.jpg";
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

TEST(Protocol, SetLEDsBatched) {
  cv::Mat parrotImage, butterflyImage;
  loadTestImage(parrotImage, ParrotTestImagePath, ParrotTestImageWidth,
                ParrotTestImageHeight);
  loadTestImage(butterflyImage, ButterflyTestImagePath, ButterflyTestImageWidth,
                ButterflyTestImageHeight);

  const size_t parrotImageSize = parrotImage.total() * parrotImage.elemSize();
  const size_t butterflyImageSize =
      butterflyImage.total() * butterflyImage.elemSize();

  constexpr size_t numImages = 2;

  // Encode

  std::shared_ptr<LEDsPixelData> parrotImagePixelData(
      reinterpret_cast<LEDsPixelData *>(
          new uint8_t[sizeof(LEDsPixelData) + parrotImageSize]));
  parrotImagePixelData->width = ParrotTestImageWidth;
  parrotImagePixelData->height = ParrotTestImageHeight;
  memcpy(parrotImagePixelData->pixel_data, parrotImage.data, parrotImageSize);

  std::vector<LEDsBatchEntryData> entries(numImages);
  entries[0].gpio_pin = -1;
  entries[0].num_leds = parrotImage.total();
  entries[0].matrices = {parrotImagePixelData};

  std::shared_ptr<LEDsPixelData> butterflyImagePixelData(
      reinterpret_cast<LEDsPixelData *>(
          new uint8_t[sizeof(LEDsPixelData) + butterflyImageSize]));
  butterflyImagePixelData->width = ButterflyTestImageWidth;
  butterflyImagePixelData->height = ButterflyTestImageHeight;
  memcpy(butterflyImagePixelData->pixel_data, butterflyImage.data,
         butterflyImageSize);

  entries[1].gpio_pin = -2;
  entries[1].num_leds = butterflyImage.total();
  entries[1].matrices = {butterflyImagePixelData};

  std::vector<uint8_t> buffer = encode_set_leds_batched(entries, RGB_24);

  const size_t expectedMsgSize =
      sizeof(SetLEDsBatchedMessageHeader) + 2 * sizeof(LEDsBatchEntryHeader) +
      2 * sizeof(LEDsPixelData) + parrotImageSize + butterflyImageSize;

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
  ASSERT_THAT(parrotEntry->header.num_matrices, Eq(1));
  const LEDsPixelData *parrotEntryPixelData = parrotEntry->matrices;
  ASSERT_THAT(parrotEntryPixelData->width, Eq(ParrotTestImageWidth));
  ASSERT_THAT(parrotEntryPixelData->height, Eq(ParrotTestImageHeight));
  // No conversion
  ASSERT_THAT(0 == std::memcmp(parrotEntryPixelData->pixel_data,
                               parrotImage.data, parrotImageSize),
              IsTrue());

  p += sizeof(LEDsBatchEntryHeader) + sizeof(LEDsPixelData) + parrotImageSize;

  const auto *butterflyEntry = reinterpret_cast<const LEDsBatchEntry *>(p);
  ASSERT_THAT(butterflyEntry->header.gpio_pin, Eq(-2));
  ASSERT_THAT(butterflyEntry->header.num_leds, Eq(butterflyImage.total()));
  ASSERT_THAT(butterflyEntry->header.num_matrices, Eq(1));
  const LEDsPixelData *butterflyEntryPixelData = butterflyEntry->matrices;
  ASSERT_THAT(butterflyEntryPixelData->width, Eq(ButterflyTestImageWidth));
  ASSERT_THAT(butterflyEntryPixelData->height, Eq(ButterflyTestImageHeight));
  // No conversion
  ASSERT_THAT(0 == std::memcmp(butterflyEntryPixelData->pixel_data,
                               butterflyImage.data, butterflyImageSize),
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

TEST(ImageEncoding, IdentityConversionRGB24) {
  const std::vector<uint8_t> src = {
      0xFF, 0x00, 0xAA, // Pixel 0: R=255, G=0, B=170
      0x12, 0x34, 0x56  // Pixel 1: R=18,  G=52, B=86
  };
  std::vector<uint8_t> dest(src.size(), 0x00);

  const size_t bytes_written =
      convert_image_encoding(2, 1, src.data(), RGB_24, dest.data(), RGB_24);

  ASSERT_THAT(bytes_written, Eq(src.size()));
  ASSERT_THAT(dest, ElementsAreArray(src));
}

TEST(ImageEncoding, ZeroLedsReturnsZeroBytes) {
  const std::vector<uint8_t> src = {0xFF, 0xFF, 0xFF};
  std::vector<uint8_t> dest(3, 0xAA);

  const size_t bytes_written =
      convert_image_encoding(0, 0, src.data(), RGB_24, dest.data(), RGB_12);

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
      convert_image_encoding(2, 1, src.data(), RGB_24, dest.data(), RGB_12);

  ASSERT_THAT(bytes_written, Eq(expected_size));
  ASSERT_THAT(dest, ElementsAre(0xAF, 0xC5, 0x38));
}

TEST(ImageEncoding, RGB12ToRGB24Unpacking) {
  const std::vector<uint8_t> src = {0xAF, 0xC5, 0x38};

  constexpr size_t expected_size = 6;
  std::vector<uint8_t> dest(expected_size, 0x00);

  const size_t bytes_written =
      convert_image_encoding(2, 1, src.data(), RGB_12, dest.data(), RGB_24);

  ASSERT_THAT(bytes_written, Eq(expected_size));
  ASSERT_THAT(dest, ElementsAre(0xFF, 0xAA, 0x55, 0xCC, 0x88, 0x33));
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

    size_t bytes_written = convert_image_encoding(
        num_leds, 1, src.data(), RGB_24, conv.data(), encoding);

    ASSERT_THAT(bytes_written, Eq(expected_size));

    std::vector<uint8_t> final(6, 0x00);

    bytes_written = convert_image_encoding(num_leds, 1, conv.data(), encoding,
                                           final.data(), RGB_24);
    ASSERT_THAT(bytes_written, Eq(6));

    std::vector<uint8_t> expected_result = src;
    for (uint8_t &v : expected_result) {
      const uint8_t channel_bits = bits_per_pixel / 3;
      const uint16_t channel_max_value = (1U << channel_bits) - 1;
      const uint8_t offset = 8 - channel_bits;

      v >>= offset;
      v = (static_cast<uint16_t>(v) * 255U + channel_max_value / 2U) / channel_max_value;
    }

    ASSERT_THAT(final, ElementsAreArray(expected_result));
  }
}

TEST(ImageEncoding, RGB24ToFromYUV444) {
  const std::vector<uint8_t> src = {241, 162, 83, 196, 133, 54};
  constexpr uint32_t num_leds = 2;

  const size_t expected_size = get_encoded_image_size(num_leds, YUV_444);
  ASSERT_THAT(expected_size, Eq(6));

  std::vector<uint8_t> conv(expected_size, 0x00);

  size_t bytes_written = convert_image_encoding(num_leds, 1, src.data(), RGB_24,
                                                conv.data(), YUV_444);

  ASSERT_THAT(bytes_written, Eq(expected_size));
  ASSERT_THAT(conv, ElementsAre(0xB1, 0x4B, 0xAE, 0x8F, 0x4E, 0xA6));

  std::vector<uint8_t> final(6, 0x00);

  bytes_written = convert_image_encoding(num_leds, 1, conv.data(), YUV_444,
                                         final.data(), RGB_24);
  ASSERT_THAT(bytes_written, Eq(6));

  ASSERT_THAT(final, ElementsAre(255, 171, 81, 209, 136, 47));
}

TEST(ImageEncoding, RGB24ToFromYUV422) {
  const std::vector<uint8_t> src = {241, 162, 83, 196, 133, 54};
  constexpr uint32_t num_leds = 2;

  const size_t expected_size = get_encoded_image_size(num_leds, YUV_422);
  ASSERT_THAT(expected_size, Eq(4));

  std::vector<uint8_t> conv(expected_size, 0x00);

  size_t bytes_written = convert_image_encoding(num_leds, 1, src.data(), RGB_24,
                                                conv.data(), YUV_422);

  ASSERT_THAT(bytes_written, Eq(expected_size));
  ASSERT_THAT(conv, ElementsAre(0xB1, 0x4D, 0x8F, 0xAA));

  std::vector<uint8_t> final(6, 0x00);

  bytes_written = convert_image_encoding(num_leds, 1, conv.data(), YUV_422,
                                         final.data(), RGB_24);
  ASSERT_THAT(bytes_written, Eq(6));

  ASSERT_THAT(final, ElementsAre(255, 173, 85, 215, 134, 45));
}

TEST(ImageEncoding, YUV444ToFromYUV420) {
  const std::vector<uint8_t> src = {
      // clang-format off

      // Y, U, V

      // --- ROW1 ---

      115, 134, 188,
      106, 134, 188,

      200, 144, 168,
      210, 144, 168,

      // --- ROW2 ---

      120, 134, 188,
      100, 134, 188,

      197, 144, 168,
      209, 144, 168,

      // --- ROW3 ---

      151, 154, 178,
      160, 154, 178,

      80, 164, 138,
      82, 164, 138,

      // --- ROW4 ---

      149, 154, 178,
      161, 154, 178,

      83, 164, 138,
      84, 164, 138,

      // clang-format on
  };

  constexpr uint32_t num_leds = 16;

  const size_t expected_size = get_encoded_image_size(num_leds, YUV_420);
  ASSERT_THAT(expected_size, Eq(24));

  std::vector<uint8_t> conv(expected_size, 0x00);

  size_t bytes_written =
      convert_image_encoding(4, 4, src.data(), YUV_444, conv.data(), YUV_420);
  ASSERT_THAT(bytes_written, Eq(expected_size));
  // 4 luma, 2 chroma
  ASSERT_THAT(conv, ElementsAre(
                        // --- LUMA ---
                        115, 106, 200, 210, // ROW1
                        120, 100, 197, 209, // ROW2
                        151, 160, 80, 82,   // ROW3
                        149, 161, 83, 84,   // ROW4
                        // --- CHROMA ---
                        134, 188, 144, 168, 154, 178, 164, 138));

  std::vector<uint8_t> final(48, 0x00);

  bytes_written =
      convert_image_encoding(4, 4, conv.data(), YUV_420, final.data(), YUV_444);
  ASSERT_THAT(bytes_written, Eq(48));

  ASSERT_THAT(final, ElementsAreArray(src));
}

TEST(ImageEncoding, ParrotConversionGamut) {
  cv::Mat parrotImage;
  loadTestImage(parrotImage, ParrotTestImagePath, ParrotTestImageWidth,
                ParrotTestImageHeight);

  constexpr size_t maxSize = ParrotTestImageWidth * ParrotTestImageHeight * 3;
  std::vector<uint8_t> conv(maxSize, 0x00);

  std::filesystem::path outputDirPath =
      std::filesystem::temp_directory_path() / "parrot";
  ASSERT_THAT(std::filesystem::create_directory(outputDirPath), IsTrue());

  const size_t parrotImageSize = parrotImage.total() * parrotImage.elemSize();
  ASSERT_THAT(parrotImageSize, Eq(maxSize));

  cv::Mat convertedParrotImage(ParrotTestImageHeight, ParrotTestImageWidth,
                               CV_8UC3);

  for (int encoding_int = static_cast<int>(RGB_24);
       encoding_int <= static_cast<int>(YUV_420); encoding_int++) {
    const auto encoding = static_cast<ImageEncoding>(encoding_int);

    const size_t expected_size = get_encoded_image_size(
        ParrotTestImageWidth * ParrotTestImageHeight, encoding);

    size_t bytes_written =
        convert_image_encoding(ParrotTestImageWidth, ParrotTestImageHeight,
                               parrotImage.data, RGB_24, conv.data(), encoding);
    ASSERT_THAT(bytes_written, Eq(expected_size));

    bytes_written = convert_image_encoding(
        ParrotTestImageWidth, ParrotTestImageHeight, conv.data(), encoding,
        convertedParrotImage.data, RGB_24);
    ASSERT_THAT(bytes_written, Eq(parrotImageSize));

    cv::cvtColor(convertedParrotImage, convertedParrotImage, cv::COLOR_RGB2BGR);
    const std::string imageName = encoding_to_string(encoding);

    const std::string imageTitle = "Parrot: RGB to/from "s + imageName;
    inspect(convertedParrotImage, imageTitle.c_str());

    std::filesystem::path outputImagePath =
        outputDirPath / (imageName + ".png");
    ASSERT_THAT(cv::imwrite(outputImagePath, convertedParrotImage), IsTrue());
  }
}