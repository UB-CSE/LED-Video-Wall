#ifndef TEXT_ELEMENT_HPP
#define TEXT_ELEMENT_HPP

#include "../inc/canvas.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <opencv2/opencv.hpp>
#include <string>

Element renderTextToElement(const std::string &text,
                            const std::string &fontPath, int fontSize,
                            cv::Scalar textColor, int elementId = 0,
                            cv::Point position = cv::Point(0, 0));

#endif
