#include "text-render.hpp"
#include "canvas.h"
#include "opencv2/core/mat.hpp"
#include "opencv2/imgcodecs.hpp"

Element renderTextToElement(const std::string &text,
                            const std::string &fontPath, int fontSize,
                            cv::Scalar textColor, int elementId,
                            cv::Point position) {
  // FreeType initialization
  FT_Library ft;
  if (FT_Init_FreeType(&ft)) {
    std::cerr << "Error: Could not initialize FreeType library" << std::endl;
    return Element(cv::Mat(), -1);
  }
  std::cerr << "Trying to load font from: " << fontPath << std::endl;

  // load font
  FT_Face face;
  if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
    std::cerr << "Error: Failed to load font" << std::endl;
    FT_Done_FreeType(ft);
    return Element(cv::Mat(), -1);
  }

  // improve font rendering by enabling hinting and using a slightly higher
  // resolution for better antialiasing
  FT_Set_Pixel_Sizes(face, 0, fontSize * 2);

  // enable hinting for better rendering at small sizes
  FT_Int32 load_flags = FT_LOAD_RENDER | FT_LOAD_TARGET_NORMAL;

  // make a generous estimate for dimensions before cropping
  int estimatedWidth = text.length() * fontSize * 2;
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
  for (const char &c : text) {
    FT_UInt glyph_index = FT_Get_Char_Index(face, c);

    // apply kerning
    if (FT_HAS_KERNING(face) && previous && glyph_index) {
      FT_Get_Kerning(face, previous, glyph_index, FT_KERNING_DEFAULT, &kerning);
      x += kerning.x >> 6;
    }

    if (FT_Load_Glyph(face, glyph_index, load_flags) ||
        FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) {
      continue;
    }

    // update dimension trackers
    maxAscender = std::max(maxAscender, face->glyph->bitmap_top);
    maxDescender = std::max(maxDescender, (int)face->glyph->bitmap.rows -
                                              face->glyph->bitmap_top);

    // render the glyph
    FT_Bitmap bitmap = face->glyph->bitmap;
    int glyph_x = x + face->glyph->bitmap_left;
    int glyph_y = y - face->glyph->bitmap_top;

    // we can't use bitmapToMat() because FT_Bitmap is not standard.
    for (unsigned int i = 0; i < bitmap.rows; i++) {
      for (unsigned int j = 0; j < bitmap.width; j++) {
        int px = glyph_x + j;
        int py = glyph_y + i;

        if (px >= 0 && px < tempImg.cols && py >= 0 && py < tempImg.rows) {
          unsigned char alpha = bitmap.buffer[i * bitmap.width + j];

          if (alpha > 0) {
            cv::Vec4b &pixel = tempImg.at<cv::Vec4b>(py, px); // BGRA
            pixel[0] = textColor[0];
            pixel[1] = textColor[1];
            pixel[2] = textColor[2];
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
  int actualWidth = maxX + 10;
  int actualHeight = maxY - minY + 20;

  // crop to the actual size used
  cv::Rect croppedRegion(0, minY - 10, actualWidth, actualHeight);

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
  cv::resize(croppedImg, img, cv::Size(actualWidth / 2, actualHeight / 2), 0, 0,
             cv::INTER_AREA);

  // create our output element and convert
  cv::Mat rgbImg;
  cv::cvtColor(img, rgbImg, cv::COLOR_BGRA2BGR);
  Element textElement(rgbImg, elementId, position);

  return textElement;
}
