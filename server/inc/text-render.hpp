#ifndef TEXT_ELEMENT_HPP
#define TEXT_ELEMENT_HPP

#include "../inc/canvas.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <opencv2/opencv.hpp>
#include <string>

Element* renderTextToElement(const std::string &text,
                             const std::string &fontPath,
                             int fontSize,
                             cv::Scalar textColor,
                             int elementId,
                             cv::Point position,
                             double rotationDegrees = 0.0);

#endif
