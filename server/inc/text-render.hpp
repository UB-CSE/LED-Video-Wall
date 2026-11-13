#pragma once

#include "canvas.hpp"
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <string>
#include <unordered_map>

#include <ft2build.h>
#include FT_FREETYPE_H

class FontManager final {
public:
  ~FontManager();

  static FontManager &get() {
    static FontManager instance;
    return instance;
  }

  FontManager(const FontManager &) = delete;
  FontManager &operator=(const FontManager &) = delete;

  void setFontsDirPath(const std::filesystem::path &fontsDirPath) {
    m_fontsDirPath = fontsDirPath;
  }

  cv::Mat renderText(std::string_view text,
                     const std::filesystem::path &fontPath, int fontSize,
                     cv::Scalar textColor);

private:
  FontManager();
  FT_Face loadFont(const std::string &fontName);

  std::filesystem::path getFontPath(const std::string &fontName) const;
  std::filesystem::path m_fontsDirPath;

  FT_Library m_ft{};
  bool m_isInitialized = false;

  // Keep faces around.
  std::unordered_map<std::filesystem::path, FT_Face> m_faces;
};
