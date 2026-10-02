#pragma once

#include "rtmp.hpp"
#include "web-browser.hpp"
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

/**
 * Base class for all canvas elements.
 */
class Element {
  std::string m_uid;
  std::string m_name;

  cv::Point m_location{0, 0};
  cv::Point m_locationOffsetRotation{0, 0};
  int m_frameRate;
  double m_angleDegrees = 0.0;
  cv::Size m_size{0, 0};
  bool m_preserveAspectRatio;
  bool m_wasPreserveAspectRatioChanged = false;

  cv::Mat m_originalPixelMatrix; // Before rotation
  cv::Mat m_finalPixelMatrix;    // After rotation

  // Scales m_finalPixelMatrix to m_size
  void scaleFrame();
  // Rotates m_finalPixelMatrix by m_rotationDegrees
  void rotateFrame();

public:
  virtual ~Element() = default;

  Element(const Element &) = delete;
  Element &operator=(const Element &) = delete;

  static std::string NewUID();

  /**
   * Get the element's unique identifier. The UID is immutable.
   * @return UID
   */
  const std::string &getUID() const { return m_uid; }

  /**
   * Get the element's name.
   * @return Name
   */
  const std::string &getName() const { return m_name; }

  /**
   * Set the element's name.
   * @param name Name
   */
  void setName(std::string_view name) { m_name = name; }

  /**
   * Get the position of the element's top left corner on the canvas.
   * @return Location
   */
  cv::Point getLocation() const {
    return m_location + m_locationOffsetRotation;
  };

  /**
   * Get the raw position of the element's top left corner on the canvas, before
   * rotation offset is applied.
   *
   * @return Location
   */
  cv::Point getRawLocation() const { return m_location; }

  /**
   * Set the position of the element's top left corner on the canvas.
   */
  void setLocation(const cv::Point &newLocation) { m_location = newLocation; }

  /**
   * Set the element's frame rate.
   * @param frameRate Frame rate in Hz
   */
  void setFrameRate(int frameRate) { m_frameRate = frameRate; }

  /**
   * Get the element's frame rate.
   * @return Frame rate in Hz
   */
  int getFrameRate() const { return m_frameRate; };

  /**
   * Get the final pixel matrix of the element (with rotation and scale
   * applied).
   * @return Pixel matrix
   */
  const cv::Mat &getPixelMatrix() const { return m_finalPixelMatrix; };

  /**
   * Set the rotation of the element.
   * @param rotationDegrees Angle in degrees. Positive is counter-clockwise,
   * negative is clockwise.
   */
  void setRotation(double rotationDegrees);

  /**
   * Apply a rotation to the element (on top of current rotation angle).
   * @param rotationDegrees Angle in degrees. Positive is counter-clockwise,
   * negative is clockwise.
   */
  void rotateBy(double rotationDegrees);

  /**
   * Get the rotation angle of the element.
   * @return Angle in degrees.
   */
  double getRotation() const { return m_angleDegrees; }

  /**
   * Scale the element to a specified size.
   * @param size The new size, in pixels.
   */
  void setSize(cv::Size size);

  /**
   * Get the size of the element.
   * @return Size in pixels.
   */
  cv::Size getSize() const { return m_size; }

  /**
   * Disable scaling.
   */
  void resetSize() { setSize(cv::Size(0, 0)); }

  /**
   * Set whether scaling will preserve aspect ratio or not.
   * @param preserveAspectRatio True or false.
   */
  void setPreserveAspectRatio(bool preserveAspectRatio);

  /**
   * Get whether scaling will preserve aspect ratio or not.
   * @return True or false.
   */
  bool getPreserveAspectRatio() const { return m_preserveAspectRatio; }

  bool wasPreserveAspectRatioChanged() const {
    return m_wasPreserveAspectRatioChanged;
  }

  /**
   * Acquire and process the next frame.
   * @return Whether the frame has changed.
   */
  bool acquireNextFrame();

  /**
   * (Implemented by child class) Reset element state.
   */
  virtual void reset() {}

protected:
  /**
   * (Implemented by child class) Acquire the next frame.
   *
   * The implementation of this method should set the output frame parameter
   * with the new frame and return true, or return false if no frame is
   * available.
   *
   * @param output The output frame.
   * @return Whether the frame has changed.
   */
  virtual bool nextFrame([[maybe_unused]] cv::Mat &output) { return false; }

  /**
   * Set the frame. Do not call this from nextFrame() to avoid an extra copy.
   * @param frame The frame.
   */
  void setFrame(const cv::Mat &frame);

  Element(std::string_view uid, int frameRate, bool preserveAspectRatio = true);

private:
  /**
   * Copy the original pixel matrix to the final one and apply scale & rotation.
   */
  void refreshFrame();
};

/**
 * Displays an image loaded from disk.
 */
class ImageElement final : public Element {
  bool m_isLoaded = false;
  std::filesystem::path m_filePath;

  // Display blue when no image loaded. Size is likely meaningless because scale
  // will most likely be applied.
  const cv::Mat kNoFrameMat = cv::Mat(100, 100, CV_8UC3, cv::Scalar(0, 0, 255));

public:
  ImageElement(std::string_view uid, const std::filesystem::path &filepath);
  ~ImageElement() override = default;

  void loadImageFile(const std::filesystem::path &path);
  const std::filesystem::path &getImageFilePath() const { return m_filePath; }

  bool isLoaded() const { return m_isLoaded; }

  // reset() does nothing
};

/**
 * Cycles through multiple images loaded from disk.
 */
class CarouselElement final : public Element {
  std::vector<std::string> m_filepaths;

  std::vector<cv::Mat> m_pixelMatrices;
  size_t m_currentIndex = 0;

  bool m_isLoaded = false;

public:
  CarouselElement(std::string_view uid, std::span<const std::string> filepaths,
                  int frameRate);
  ~CarouselElement() override = default;

  void loadImages(std::span<const std::string> filepaths);
  const std::vector<std::string> &getImageFilePaths() const {
    return m_filepaths;
  }

  bool isLoaded() const { return m_isLoaded; }

  void reset() override;

private:
  bool nextFrame(cv::Mat &output) override;
};

class VideoElement final : public Element {
  std::filesystem::path m_filepath;

  cv::VideoCapture m_cap;

  bool m_isLoaded = false;

  // Display blue when no video loaded. Size is likely meaningless because scale
  // will most likely be applied.
  const cv::Mat kNoFrameMat = cv::Mat(100, 100, CV_8UC3, cv::Scalar(0, 0, 255));

public:
  VideoElement(std::string_view uid, const std::filesystem::path &filepath,
               int frameRate);
  ~VideoElement() override = default;

  void loadVideo(const std::filesystem::path &filepath);
  const std::filesystem::path &getVideoFilePath() const { return m_filepath; }

  void reset() override;

private:
  bool nextFrame(cv::Mat &output) override;
};

class RTMPStreamElement final : public Element {
  RTMPServer &m_rtmpServer;
  std::string m_streamName;

  const cv::Mat kNoFrameMat =
      cv::Mat(100, 100, CV_8UC3, cv::Scalar(0, 255, 0)); // green

public:
  RTMPStreamElement(std::string_view uid, RTMPServer &rtmpServer,
                    std::string_view streamName, int frameRate);
  ~RTMPStreamElement() override = default;

  void setStreamName(std::string_view streamName);
  std::string_view getStreamName() const { return m_streamName; }

  void reset() override;

private:
  bool nextFrame(cv::Mat &output) override;
};

class WebBrowserElement final : public Element {
  std::string m_url;
  cv::Size m_viewSize;
  WebBrowser m_webBrowser;

  const cv::Mat kNoFrameMat =
      cv::Mat(100, 100, CV_8UC3, cv::Scalar(255, 0, 0)); // red

  std::vector<CefCookie> m_cookies;

public:
  /**
   * Construct a WebBrowserElement.
   * @param uid Element UID.
   * @param url URL of website to display.
   * @param frameRate Frame rate (Hz)
   * @param viewSize Size of the browser window. Probably should be much larger
   *                 than the element size on the canvas to make webpages render
   *                 correctly. Scale the element with setSize()
   */
  WebBrowserElement(std::string_view uid, std::string_view url, int frameRate,
                    cv::Size viewSize);
  ~WebBrowserElement() override = default;

  void setURL(std::string_view url);
  std::string_view getURL() const { return m_url; }

  void setViewSize(cv::Size viewSize);
  cv::Size getViewSize() const { return m_viewSize; }

  void reset() override;

  void
  setCookie(std::string_view name, std::string_view value,
            std::string_view domain, std::string_view path, bool secure = false,
            bool httpOnly = false,
            cef_cookie_same_site_t sameSite = CEF_COOKIE_SAME_SITE_UNSPECIFIED);

  std::span<const CefCookie> getCookies() const { return m_cookies; }

private:
  bool nextFrame(cv::Mat &output) override;
};

class TextElement final : public Element {
  std::string m_text;
  std::filesystem::path m_fontPath;
  int m_fontSize;
  cv::Scalar m_color;

public:
  TextElement(std::string_view uid, std::string_view text,
              const std::filesystem::path &fontPath, int fontSize,
              cv::Scalar col);
  ~TextElement() override = default;

  void setText(std::string_view text);
  std::string_view getText() const { return m_text; }

  void setFont(const std::filesystem::path &fontPath);
  const std::filesystem::path &getFontPath() const { return m_fontPath; }

  void setFontSize(int fontSize);
  int getFontSize() const { return m_fontSize; }

  void setColor(const cv::Scalar &color);
  const cv::Scalar &getColor() const { return m_color; }

private:
  void render();
};

class VirtualCanvas final {
  cv::Mat m_pixelMatrix;
  double m_gamma = 1.0;
  cv::Mat m_canvasLUT = cv::Mat(1, 256, CV_8UC1);
  cv::Size m_dim;

  using ElementPtr = std::shared_ptr<Element>;
  using List = std::list<ElementPtr>;
  using Iterator = List::iterator;

  List m_elements; // Insert & remove are O(1)
  std::unordered_map<std::string, Iterator>
      m_elementPositions; // Make access O(1)

public:
  explicit VirtualCanvas(const cv::Size &size);
  ~VirtualCanvas() = default;

  VirtualCanvas(const VirtualCanvas &) = delete;
  VirtualCanvas &operator=(const VirtualCanvas &) = delete;

  cv::Size getDimensions() const { return m_dim; }

  /**
   * Less than 1 -> lighter, More than 1 -> Darker
   * @param gamma Gamma
   */
  void setGamma(double gamma);

  /**
   * Populates canvas elements from a canvas configuration YAML file.
   * @param path Config file path.
   * @param rtmpServer RTMP server used for constructing RTMP stream elements.
   * @return True if successful, false otherwise.
   */
  bool loadElementConfig(const std::filesystem::path &path,
                         RTMPServer &rtmpServer);

  /**
   * Write the current canvas state to a YAML configuration file.
   * @param path Config file path.
   */
  void saveElementConfig(const std::filesystem::path &path) const;

  /**
   * Remove all elements from the canvas.
   */
  void clearElements();

  /**
   * Return the number of elements on the canvas.
   * @return Element count.
   */
  size_t getElementCount() const { return m_elements.size(); }

  /**
   * Get the canvas elements.
   * @return Elements list.
   */
  const List &getElements() const { return m_elements; }

  /**
   * Get an element on the canvas by its UID.
   * @param uid Element unique identifier.
   * @return Pointer to the element, or nullptr if not found.
   */
  std::shared_ptr<Element> getElement(const std::string &uid);

  /**
   * Get an element on the canvas by its UID.
   * @param uid Element unique identifier.
   * @return Pointer to the element, or nullptr if the element was not found.
   */
  std::shared_ptr<const Element> getElement(const std::string &uid) const;

  /**
   * Add an element to the canvas.
   * @param element Element to add.
   * @return True if successful, false if an element with the same UID is
   *         already on the canvas.
   */
  bool addElement(std::shared_ptr<Element> element);

  /**
   * Remove an element from the canvas by its UID.
   * @param uid Element unique identifier.
   * @return True if successful, false if the element was not found.
   */
  bool removeElement(const std::string &uid);

  /**
   * Increase the Z position of an element on the canvas.
   * @param uid Element unique identifier.
   * @return True if successful, false if the element was not found.
   */
  bool moveElementUp(const std::string &uid);

  /**
   * Decrease the Z position of an element on the canvas.
   * @param uid Element unique identifier.
   * @return True if successful, false if the element was not found.
   */
  bool moveElementDown(const std::string &uid);

  /**
   * Get the rendered canvas pixel matrix.
   * @return Pixel matrix.
   */
  const cv::Mat &getPixelMatrix() const { return m_pixelMatrix; }

  /**
   * Clear the canvas pixel matrix.
   */
  void clearPixelMatrix() { m_pixelMatrix = cv::Mat::zeros(m_dim, CV_8UC3); }

  /**
   * Render the canvas elements to the pixel matrix.
   */
  void pushElementsToPixelMatrix();

private:
  void overlayImage(const cv::Mat &overlay, cv::Rect roi);
};