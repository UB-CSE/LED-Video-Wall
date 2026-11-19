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

#pragma region Resource File Paths

static const std::filesystem::path RainbowTestImagePath =
    TestImagesDir / "223924eb4e7f1b971ecc1466c0f3030ebfa20eb7e73e56ee30fd9d00caf62421.png";
static constexpr int RainbowTestImageWidth = 512;
static constexpr int RainbowTestImageHeight = 320;

static const std::filesystem::path ParrotTestImagePath =
    TestImagesDir / "84d6da9b5559a803213bac93a03a9bc82465b64b558fd61866acc550b3ba697f.jpg";
static constexpr int ParrotTestImageWidth = 150;
static constexpr int ParrotTestImageHeight = 200;

static const std::filesystem::path ButterflyTestImagePath =
    TestImagesDir / "c0a7a30d78b23cf5f60d393d601dfc823ce2650118978aa01bcf4e5dedbd1e28.jpg";
static constexpr int ButterflyTestImageWidth = 256;
static constexpr int ButterflyTestImageHeight = 256;

static const std::filesystem::path AlanTestVideoPath =
    TestImagesDir / "alan" / "alan_ca_4_64x64.mp4";

static const std::filesystem::path ConwayTestVideoPath =
    TestImagesDir / "conway_pulsar.mp4";

static const std::filesystem::path RobotoFontPath =
    TestFontsDir / "Roboto-Regular.ttf";

static const std::filesystem::path LobsterFontPath =
    TestFontsDir / "Lobster-Regular.ttf";

#pragma endregion

/**
 * Returns a string identifying the current running test.
 * @return Identifier
 */
std::string getCurrentTestID();

/**
 * Returns a string identifying the test with the given TestInfo.
 * @param info Test info
 * @return Identifier
 */
std::string getTestID(const TestInfo &info);

/**
 * Get the path to a directory for the test to write output files to for
 * inspection later.
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

/**
 * Returns the string contents of a file.
 * @param path File path
 * @return Contents
 */
std::string readFile(const std::filesystem::path &path);
