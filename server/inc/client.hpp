#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <memory>
#include <string>
#include <vector>
#include <cstdint>
#include <optional>
#include <opencv2/opencv.hpp>

enum rotation { UP, DOWN, LEFT, RIGHT };

class CanvasPos {
public:
    uint32_t x;
    uint32_t y;
    // width and height for CanvasPos should not be confused with those in LEDMAtrixSpec,
    // these may be flipped depending on rotation.
    uint32_t width;
    uint32_t height;
    rotation rot;

    CanvasPos(uint32_t x,
              uint32_t y,
              uint32_t width,
              uint32_t height,
              rotation rot);
};

class LEDMatrixSpec {
public:
    std::string id;
    float power_limit_amps;
    uint32_t width;
    uint32_t height;
    uint64_t total_leds;

    LEDMatrixSpec(std::string id,
                  float power_limit_amps,
                  uint32_t width,
                  uint32_t height);

    std::string to_string();
};

class LEDMatrix {
public:
    std::string id;
    LEDMatrixSpec* spec;
    CanvasPos pos;
    uint32_t packed_pixel_array_size;
    unsigned char* packed_pixel_array;

    LEDMatrix(std::string id,
              LEDMatrixSpec* spec,
              CanvasPos pos);

    std::string to_string();
};

class MatricesConnection {
public:
    uint8_t pin;
    std::vector<LEDMatrix*> matrices;

    std::string to_string();
};

class Client {
public:
    uint64_t mac_addr;
    std::vector<MatricesConnection> mat_connections;

    Client(uint64_t mac_addr,
           std::vector<MatricesConnection> mat_connections);

    std::string to_string();
};

#endif
