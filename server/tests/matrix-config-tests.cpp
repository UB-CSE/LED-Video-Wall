#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <tests-config.hpp>

#include "matrix-config.hpp"

using namespace testing;

static const std::filesystem::path ConfigPath =
    TestResourcesDir / "matrix-configs" / "config.yaml";

namespace {

const LEDMatrixSpec MatrixWS2812B_32x8{
    .id = "ws2812b:32x8",
    .powerLimitAmps = 2.5,
    .width = 8,
    .height = 32,
};

const LEDMatrixSpec MatrixP3_64x64{
    .id = "p3:64x64",
    .powerLimitAmps = 5.0,
    .width = 64,
    .height = 64,
};

void VerifyLEDMatrixSpec(const LEDMatrixSpec &actual,
                         const LEDMatrixSpec &expected) {
  ASSERT_THAT(actual.id, StrEq(expected.id));
  ASSERT_THAT(actual.powerLimitAmps, FloatEq(expected.powerLimitAmps));
  ASSERT_THAT(actual.width, Eq(expected.width));
  ASSERT_THAT(actual.height, Eq(expected.height));
}

const LEDMatrix ExpectedMat1{
    .id = "mat1",
    .spec = std::make_shared<LEDMatrixSpec>(MatrixWS2812B_32x8),
    .pos = {.x = 0, .y = 0, .width = 8, .height = 32, .rot = Rotation::DOWN},
};

const LEDMatrix ExpectedMat2{
    .id = "mat2",
    .spec = std::make_shared<LEDMatrixSpec>(MatrixWS2812B_32x8),
    .pos = {.x = 8, .y = 0, .width = 8, .height = 32, .rot = Rotation::DOWN},
};

const LEDMatrix ExpectedMat3{
    .id = "mat3",
    .spec = std::make_shared<LEDMatrixSpec>(MatrixWS2812B_32x8),
    .pos = {.x = 16, .y = 0, .width = 8, .height = 32, .rot = Rotation::DOWN},
};

const LEDMatrix ExpectedMat4{
    .id = "mat4",
    .spec = std::make_shared<LEDMatrixSpec>(MatrixWS2812B_32x8),
    .pos = {.x = 24, .y = 0, .width = 8, .height = 32, .rot = Rotation::DOWN},
};

const LEDMatrix ExpectedMat5{
    .id = "mat5",
    .spec = std::make_shared<LEDMatrixSpec>(MatrixP3_64x64),
    .pos = {.x = 0, .y = 0, .width = 64, .height = 64, .rot = Rotation::DOWN},
};

const LEDMatrix ExpectedMat6{
    .id = "mat6",
    .spec = std::make_shared<LEDMatrixSpec>(MatrixP3_64x64),
    .pos = {.x = 0, .y = 64, .width = 64, .height = 64, .rot = Rotation::DOWN},
};

void VerifyLEDMatrix(const LEDMatrix &actual, const LEDMatrix &expected) {
  ASSERT_THAT(actual.id, StrEq(expected.id));
  ASSERT_THAT(actual.spec, NotNull());
  VerifyLEDMatrixSpec(*actual.spec, *expected.spec);
}

void VerifyMatricesConnection(const MatricesConnection &actual,
                              const MatricesConnection &expected) {
  ASSERT_THAT(actual.pin, Eq(expected.pin));
  ASSERT_THAT(actual.matrices, SizeIs(expected.matrices.size()));
  for (size_t i = 0; i < expected.matrices.size(); i++) {
    std::shared_ptr<LEDMatrix> mat = actual.matrices[i];
    VerifyLEDMatrix(*mat, *expected.matrices[i]);
  }
}

void VerifyClient(const Client &actual, const Client &expected) {
  ASSERT_THAT(actual.macAddr, Eq(expected.macAddr));
  ASSERT_THAT(actual.matConnections, SizeIs(expected.matConnections.size()));
  for (size_t i = 0; i < expected.matConnections.size(); i++) {
    VerifyMatricesConnection(actual.matConnections[i],
                             expected.matConnections[i]);
  }
}

const Client ExpectedClients[] = {
    Client{
        .macAddr = 0xD4'05'26'F7'C6'30,
        .matConnections =
            {
                MatricesConnection{
                    .pin = 16,
                    .matrices = {std::make_shared<LEDMatrix>(ExpectedMat1)}},
                MatricesConnection{
                    .pin = 17,
                    .matrices = {std::make_shared<LEDMatrix>(ExpectedMat2)}},
                MatricesConnection{
                    .pin = 18,
                    .matrices = {std::make_shared<LEDMatrix>(ExpectedMat3)}},
                MatricesConnection{
                    .pin = 19,
                    .matrices = {std::make_shared<LEDMatrix>(ExpectedMat4)}},
            },
    },
    {
        .macAddr = 0x6C'56'89'AC'85'D8,
        .matConnections =
            {
                MatricesConnection{
                    .pin = -1,
                    .matrices = {std::make_shared<LEDMatrix>(ExpectedMat5)}},
                MatricesConnection{
                    .pin = -2,
                    .matrices = {std::make_shared<LEDMatrix>(ExpectedMat6)}},
            },
    },
};

} // namespace

TEST(MatrixConfig, Basic) {
  MatrixConfig config;
  bool result = config.load(ConfigPath);
  ASSERT_THAT(result, IsTrue());

  ASSERT_THAT(config.brightness_percent, FloatEq(97));
  ASSERT_THAT(config.image_encoding, Eq(ImageEncoding::RGB_18));
  ASSERT_THAT(config.ns_per_frame, Eq(40000000));
  ASSERT_THAT(config.canvas_size.width, Eq(64));
  ASSERT_THAT(config.canvas_size.height, Eq(128));
  ASSERT_THAT(config.ledvwPort, Eq(7070));
  ASSERT_THAT(config.rtmpPort, Eq(1935));

  ASSERT_THAT(config.clients, SizeIs(2));
  for (size_t i = 0; i < 2; i++) {
    std::shared_ptr<Client> client = config.clients[i];
    ASSERT_THAT(client, NotNull());

    VerifyClient(*client, ExpectedClients[i]);

    const MatricesConnection &matConn = client->matConnections[i];
    ASSERT_THAT(matConn.matrices, SizeIs(1));
  }
}
