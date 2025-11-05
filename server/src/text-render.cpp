#include "text-render.hpp"
#include "canvas.hpp"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <iostream>
#include <algorithm>

Element* renderTextToElement(const std::string &text,
                             const std::string &fontPath, int fontSize,
                             cv::Scalar textColor, int elementId,
                             cv::Point position) {
  // FreeType initialization
  FT_Library ft;
  if (FT_Init_FreeType(&ft)) {
    std::cerr << "Error: Could not initialize FreeType library" << std::endl;
    return nullptr;
  }
  std::cerr << "Trying to load font from: " << fontPath << std::endl;

  // load font
  FT_Face face;
  if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
    std::cerr << "Error: Failed to load font" << std::endl;
    FT_Done_FreeType(ft);
    return nullptr;
  }

  // improve font rendering by enabling hinting and using a slightly higher
  // resolution for better antialiasing
  FT_Set_Pixel_Sizes(face, 0, fontSize * 2);

  // enable hinting for better rendering at small sizes
  FT_Int32 load_flags = FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL;

  // make a generous estimate for dimensions before cropping
  int estimatedWidth = static_cast<int>(text.length()) * fontSize * 2;
  int estimatedHeight = fontSize * 4;

  cv::Mat tempImg(estimatedHeight, estimatedWidth, CV_8UC4,
                  cv::Scalar(0, 0, 0, 0));

  int maxX = 0;
  int maxY = 0;
  int minY = estimatedHeight;
  int maxAscender = 0;
  int maxDescender = 0;

  // generate reasonable baseline position with some padding
  int x = 10;
  int y = estimatedHeight / 2;

  FT_UInt previous = 0;
  FT_Vector kerning;

  // begin processing text
  for (unsigned char c : text) {
    FT_UInt glyph_index = FT_Get_Char_Index(face, static_cast<FT_ULong>(c));

    // apply kerning
    if (FT_HAS_KERNING(face) && previous && glyph_index) {
      FT_Get_Kerning(face, previous, glyph_index, FT_KERNING_DEFAULT, &kerning);
      x += kerning.x >> 6;
    }

    if (FT_Load_Glyph(face, glyph_index, load_flags) ||
        FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) {
      previous = glyph_index;
      continue;
    }

    // update dimension trackers
    maxAscender = std::max(maxAscender, face->glyph->bitmap_top);
    maxDescender = std::max(maxDescender, static_cast<int>(face->glyph->bitmap.rows) -
                                              face->glyph->bitmap_top);

    // render the glyph
    FT_Bitmap bitmap = face->glyph->bitmap;
    int glyph_x = x + face->glyph->bitmap_left;
    int glyph_y = y - face->glyph->bitmap_top;

    // we can't use bitmapToMat() because FT_Bitmap is not standard.
    for (unsigned int i = 0; i < bitmap.rows; i++) {
      for (unsigned int j = 0; j < bitmap.width; j++) {
        int px = glyph_x + static_cast<int>(j);
        int py = glyph_y + static_cast<int>(i);

        if (px >= 0 && px < tempImg.cols && py >= 0 && py < tempImg.rows) {
          unsigned char alpha = bitmap.buffer[i * bitmap.width + j];

          if (alpha > 0) {
            cv::Vec4b &pixel = tempImg.at<cv::Vec4b>(py, px); // BGRA
            pixel[0] = static_cast<uchar>(textColor[0]);
            pixel[1] = static_cast<uchar>(textColor[1]);
            pixel[2] = static_cast<uchar>(textColor[2]);
            pixel[3] = alpha;

            // track the boundaries of actual pixels
            maxX = std::max(maxX, px);
            maxY = std::max(maxY, py);
            minY = std::min(minY, py);
          }
        }
      }
    }

    // advance position
    x += (face->glyph->advance.x >> 6);
    previous = glyph_index;
  }

  // calculate actual dimensions with some padding
  int actualWidth = std::max(1, maxX + 10);
  int actualHeight = std::max(1, maxY - minY + 20);

  // crop to the actual size used
  cv::Rect croppedRegion(0, std::max(0, minY - 10), actualWidth, actualHeight);

  // ensure roi stays within image bounds
  croppedRegion.x = std::max(0, croppedRegion.x);
  croppedRegion.y = std::max(0, croppedRegion.y);
  croppedRegion.width =
      std::min(croppedRegion.width, tempImg.cols - croppedRegion.x);
  croppedRegion.height =
      std::min(croppedRegion.height, tempImg.rows - croppedRegion.y);

  cv::Mat croppedImg = tempImg(croppedRegion).clone();

  // FreeType clean up
  FT_Done_Face(face);
  FT_Done_FreeType(ft);

  // downsample the cropped image to get better antialiasing
  cv::Mat img;
  cv::resize(croppedImg, img,
             cv::Size(std::max(1, actualWidth / 2), std::max(1, actualHeight / 2)),
             0, 0, cv::INTER_AREA);

  // create our output element and convert
  cv::Mat rgbImg;
  cv::cvtColor(img, rgbImg, cv::COLOR_BGRA2BGR);

  if (rgbImg.empty()) {
    return nullptr;
  }

  // return a concrete element pointer
  return new TextElement(rgbImg, elementId, position, /*frameRate*/ 0);
}
