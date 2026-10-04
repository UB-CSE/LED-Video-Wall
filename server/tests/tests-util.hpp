#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <tests-config.hpp>

#include <filesystem>
#include <opencv2/opencv.hpp>
#include <string>

using namespace testing;

#pragma region Test Configuration

/**
 * When defined, tests will pause and display images in a GUI window for manual
 * inspection.
 * DO NOT COMMIT WITH THIS ENABLED.
 */
// #define MANUAL_INSPECTION

#pragma endregion

/**
 * Returns a string identifying the current running test.
 * @return Identifier
 */
std::string getCurrentTestID();

/**
 * Get the path to a directory for the test to write output files to for inspection later.
 * @return Path
 */
std::filesystem::path getTestOutputDirPath();

/**
 * Save an image to the test output directory, for inspection later.
 * @param image Image matrix.
 * @param name Image name.
 */
void saveImage(const cv::Mat &image, std::string name);

/**
 * Pause the test and display an image to the user for manual inspection.
 * @param image Image matrix.
 * @param title Window title.
 */
void inspect(const cv::Mat &image, std::string title = "");
