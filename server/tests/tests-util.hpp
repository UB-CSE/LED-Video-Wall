#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <tests-config.hpp>

#include <opencv2/opencv.hpp>

// When defined, tests will display the canvas to inspect.
// #define MANUAL_INSPECTION

static void inspect(const cv::Mat &image, const char *title = "") {
#ifdef MANUAL_INSPECTION
  cv::imshow(title, image);
  cv::waitKey(0);
#endif
}
