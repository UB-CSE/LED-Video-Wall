#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <opencv2/opencv.hpp>

#include <string>
#include <vector>
#include <cstdint>
#include <optional>

enum rotation { UP, DOWN, LEFT, RIGHT };

class CanvasPos {
public:
    uint32_t x;
    uint32_t y;
    rotation rot;

    CanvasPos(uint32_t x,
              uint32_t y,
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
    int socket;
    std::vector<MatricesConnection> mat_connections;

    Client(uint64_t mac_addr,
           int socket,
           std::vector<MatricesConnection> mat_connections);

    std::string to_string();

    void set_leds_all_matrices(const cv::Mat &cvmat);
};

#endif
