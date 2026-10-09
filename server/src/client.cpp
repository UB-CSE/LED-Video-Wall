#include "client.hpp"

const char *RotationToString(Rotation rot) {
  switch (rot) {
  case Rotation::UP:
    return "up";
  case Rotation::DOWN:
    return "down";
  case Rotation::LEFT:
    return "left";
  case Rotation::RIGHT:
    return "right";
  default:
    return "unknown";
  }
}

Rotation RotationFromString(std::string_view str) {
  if (str == "up") {
    return Rotation::UP;
  } else if (str == "down") {
    return Rotation::DOWN;
  } else if (str == "left") {
    return Rotation::LEFT;
  } else if (str == "right") {
    return Rotation::RIGHT;
  }
  return Rotation::UNKNOWN;
}
