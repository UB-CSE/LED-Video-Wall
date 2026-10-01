#pragma once

#include <cstdint>
#include <memory>
#include <opencv2/opencv.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

enum class Rotation { UNKNOWN = -1, UP, DOWN, LEFT, RIGHT };

const char *RotationToString(Rotation rot);
Rotation RotationFromString(std::string_view str);

struct CanvasPos {
  uint32_t x;
  uint32_t y;

  // width and height for CanvasPos should not be confused with those in
  // LEDMatrixSpec, these may be flipped depending on rotation.
  uint32_t width;
  uint32_t height;

  Rotation rot;
};

struct LEDMatrixSpec {
  std::string id;
  float powerLimitAmps;
  uint32_t width;
  uint32_t height;

  uint32_t getTotalLEDs() const { return width * height; }
};

struct LEDMatrix {
  std::string id;
  std::shared_ptr<LEDMatrixSpec> spec;
  CanvasPos pos;

  uint32_t getRGB24PixelArraySize() const { return spec->getTotalLEDs() * 3; }
};

struct MatricesConnection {
  int8_t pin;
  std::vector<std::shared_ptr<LEDMatrix>> matrices;
};

struct Client {
  uint64_t macAddr;
  std::vector<MatricesConnection> matConnections;
};
