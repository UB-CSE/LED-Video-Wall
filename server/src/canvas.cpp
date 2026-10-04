#include "canvas.hpp"

#include "client.hpp"
#include "encoding-util.hpp"
#include "text-render.hpp"

#include <spdlog/spdlog.h>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <random>
#include <ranges>
#include <stdexcept>
#include <string>
#include <utility>

#pragma region Element

std::string Element::NewUID() {
  static constexpr char chars[] =
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

  static thread_local std::mt19937 rng{std::random_device{}()};
  std::uniform_int_distribution<std::size_t> pick(0, sizeof(chars) - 2);

  std::string uid;
  uid.reserve(10);

  for (int i = 0; i < 10; ++i) {
    uid += chars[pick(rng)];
  }

  return uid;
}

Element::Element(std::string_view uid, int frameRate,
                 bool preserveAspectRatio /*= true*/)
    : m_uid(uid), m_frameRate(frameRate),
      m_preserveAspectRatio(preserveAspectRatio) {}

void Element::scaleFrame() {
  if (m_size.empty()) {
    return;
  }

  const cv::Size originalSize = m_finalPixelMatrix.size();
  if (originalSize.empty() || m_size == originalSize) {
    return;
  }

  cv::Size newSize = m_size;

  if (m_preserveAspectRatio) { // Avoid stretching
    double aspectRatio =
        static_cast<double>(originalSize.width) / originalSize.height;
    int newWidth = m_size.width;
    int newHeight = static_cast<int>(newWidth / aspectRatio);
    if (newHeight > m_size.height) {
      newHeight = m_size.height;
      newWidth = static_cast<int>(newHeight * aspectRatio);
    }
    newSize = cv::Size(newWidth, newHeight);
  }

  cv::resize(m_finalPixelMatrix, m_finalPixelMatrix, newSize, 0, 0);
}

void Element::rotateFrame() {
  m_locationOffsetRotation = cv::Point(0, 0);

  if (m_angleDegrees == 0.f || m_finalPixelMatrix.empty()) {
    return;
  }

  cv::Size frameSize = m_finalPixelMatrix.size();

  // Calculate how much bigger the element needs to be to fit the rotated frame
  // without cropping

  double frameHalfWidth = frameSize.width / 2.0,
         frameHalfHeight = frameSize.height / 2.0;
  cv::Point2d frameCenter(frameHalfWidth, frameHalfHeight);
  double frameCenterToCornerAngleRad =
      std::atan(frameHalfHeight / frameHalfWidth);
  double frameCenterToCornerDist = std::hypot(frameHalfWidth, frameHalfHeight);

  double angleRad = m_angleDegrees * (M_PI / 180.0);

  auto extendAtAngle = [](cv::Point2d center, double dist, double theta) {
    double dx = dist * std::cos(theta);
    double dy = dist * std::sin(theta);
    return cv::Point2d(center.x + dx, center.y + dy);
  };

  cv::Point2d frameTopRightAfterRotation =
      extendAtAngle(frameCenter, frameCenterToCornerDist,
                    angleRad + frameCenterToCornerAngleRad);
  cv::Point2d frameTopLeftAfterRotation =
      extendAtAngle(frameCenter, frameCenterToCornerDist,
                    angleRad + M_PI - frameCenterToCornerAngleRad);

  double maxX = std::max(std::abs(frameCenter.x - frameTopRightAfterRotation.x),
                         std::abs(frameCenter.x - frameTopLeftAfterRotation.x));
  double maxY = std::max(std::abs(frameCenter.y - frameTopRightAfterRotation.y),
                         std::abs(frameCenter.y - frameTopLeftAfterRotation.y));

  int paddingX =
      static_cast<int>(std::ceil(std::max(maxX - frameHalfWidth, 0.0)));
  int paddingY =
      static_cast<int>(std::ceil(std::max(maxY - frameHalfHeight, 0.0)));

  // Expand the frame with a transparent border

  cv::Mat paddedFrame = cv::Mat::zeros(frameSize.height + 2 * paddingY,
                                       frameSize.width + 2 * paddingX, CV_8UC4);
  if (m_finalPixelMatrix.channels() == 3) {
    cv::cvtColor(m_finalPixelMatrix, m_finalPixelMatrix, cv::COLOR_BGR2BGRA);
  }
  m_finalPixelMatrix.copyTo(paddedFrame(
      cv::Rect(paddingX, paddingY, frameSize.width, frameSize.height)));

  cv::Size paddedFrameSize = paddedFrame.size();
  cv::Point2d paddedFrameCenter(frameHalfWidth + paddingX,
                                frameHalfHeight + paddingY);

  // Rotate around the center

  cv::Mat rotationMatrix =
      cv::getRotationMatrix2D(paddedFrameCenter, m_angleDegrees, 1.0);
  cv::warpAffine(paddedFrame, m_finalPixelMatrix, rotationMatrix,
                 paddedFrameSize);

  // Keep the center of the element in the same place on the canvas
  m_locationOffsetRotation = cv::Point(-paddingX, -paddingY);
}

void Element::setRotation(double rotationDegrees) {
  if (m_angleDegrees != rotationDegrees) {
    m_angleDegrees = rotationDegrees;
    refreshFrame();
  }
}

void Element::rotateBy(double rotationDegrees) {
  setRotation(m_angleDegrees + rotationDegrees);
}

void Element::setSize(cv::Size size) {
  // For some reason this is signed...
  if (size.width < 0)
    size.width = 0;
  if (size.height < 0)
    size.height = 0;

  if (m_size != size) {
    m_size = size;
    refreshFrame();
  }
}

void Element::setPreserveAspectRatio(bool preserveAspectRatio) {
  m_wasPreserveAspectRatioChanged = true;
  if (m_preserveAspectRatio != preserveAspectRatio) {
    m_preserveAspectRatio = preserveAspectRatio;
    refreshFrame();
  }
}

bool Element::acquireNextFrame() {
  const bool isNewFrame = nextFrame(m_originalPixelMatrix);
  if (isNewFrame) {
    refreshFrame();
  }
  return isNewFrame;
}

void Element::setFrame(const cv::Mat &frame) {
  m_originalPixelMatrix = frame.clone();
  refreshFrame();
}

void Element::refreshFrame() {
  m_finalPixelMatrix = m_originalPixelMatrix.clone();
  scaleFrame();
  rotateFrame();
}

#pragma endregion

#pragma region ImageElement

ImageElement::ImageElement(std::string_view uid,
                           const std::filesystem::path &filepath)
    : Element(uid, -1, true) {
  loadImageFile(filepath);
}

void ImageElement::loadImageFile(const std::filesystem::path &path) {
  cv::Mat frame = cv::imread(path, cv::IMREAD_UNCHANGED);
  m_filePath = path;

  m_isLoaded = !frame.empty();
  if (m_isLoaded) {
    setFrame(frame);
  } else {
    spdlog::error("[Element] {}: failed to load image at path `{}'", getUID(),
                  m_filePath.string());
    setFrame(kNoFrameMat);
  }
}

static std::shared_ptr<Element> ImageElementFromYAML(const std::string &uid,
                                                     YAML::Node node) {
  const auto filepath = node["filepath"].as<std::string>();
  return std::make_shared<ImageElement>(uid, filepath);
}

static void ImageElementToYAML(std::shared_ptr<ImageElement> element,
                               YAML::Node &node) {
  node["filepath"] = element->getImageFilePath().string();
}

#pragma endregion

#pragma region CarouselElement

CarouselElement::CarouselElement(std::string_view uid,
                                 std::span<const std::string> filepaths,
                                 int frameRate)
    : Element(uid, frameRate) {
  loadImages(filepaths);
}

void CarouselElement::loadImages(std::span<const std::string> filepaths) {
  m_filepaths = std::vector(filepaths.begin(), filepaths.end());

  m_isLoaded = true;

  for (const auto &path : filepaths) {
    cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
    if (img.empty()) {
      spdlog::error("[Element] {}: failed to load carousel image at path `{}'",
                    getUID(), path);
      m_isLoaded = false;
    } else {
      m_pixelMatrices.push_back(img);
    }
  }

  if (m_pixelMatrices.empty()) {
    spdlog::error("[Element] {}: no images loaded", getUID());
    return;
  }

  // Init current matrix with top matrix
  setFrame(m_pixelMatrices[0].clone());
}

bool CarouselElement::nextFrame(cv::Mat &output) {
  output = m_pixelMatrices[m_currentIndex];
  m_currentIndex = (m_currentIndex + 1) % m_pixelMatrices.size();
  return true;
}

void CarouselElement::reset() {
  m_currentIndex = 0;
  setFrame(m_pixelMatrices[0]);
}

static std::shared_ptr<Element> CarouselElementFromYAML(const std::string &uid,
                                                        YAML::Node node) {
  const auto filepaths = node["filepaths"].as<std::vector<std::string>>();
  const auto framerate = node["framerate"].as<int>();
  return std::make_shared<CarouselElement>(uid, filepaths, framerate);
}

static void
CarouselElementToYAML(std::shared_ptr<CarouselElement> carouselElement,
                      YAML::Node &node) {
  node["filepaths"] = carouselElement->getImageFilePaths();
  node["framerate"] = carouselElement->getFrameRate();
}

#pragma endregion

#pragma region VideoElement

VideoElement::VideoElement(std::string_view uid,
                           const std::filesystem::path &filepath, int frameRate)
    : Element(uid, frameRate) {
  loadVideo(filepath);
}

void VideoElement::loadVideo(const std::filesystem::path &filepath) {
  m_filepath = filepath;

  if (filepath.string().starts_with("rtsp://")) {
    m_cap.open(filepath, cv::CAP_FFMPEG);
  } else {
    m_cap.open(filepath);
  }

  m_isLoaded = m_cap.isOpened();
  if (m_isLoaded) {
    // Load first frame
    (void)acquireNextFrame();

  } else {
    spdlog::error("[Element] {}: failed to load video at path `{}'", getUID(),
                  filepath.string());
    setFrame(kNoFrameMat);
  }
}

bool VideoElement::nextFrame(cv::Mat &output) {
  if (!m_isLoaded) {
    return false;
  }

  bool success = m_cap.read(output);

  if (!success) {
    // Rewind and try again
    m_cap.set(cv::CAP_PROP_POS_FRAMES, 0);
    success = m_cap.read(output);
  }

  return success;
}

void VideoElement::reset() {
  m_cap.set(cv::CAP_PROP_POS_FRAMES, 0);

  // Reload first frame back into pixelMatrix
  (void)acquireNextFrame();
}

static std::shared_ptr<Element> VideoElementFromYAML(const std::string &uid,
                                                     YAML::Node node) {
  const auto filepath = node["filepath"].as<std::string>();
  const auto framerate = node["framerate"].as<int>();
  return std::make_shared<VideoElement>(uid, filepath, framerate);
}

static void VideoElementToYAML(std::shared_ptr<VideoElement> videoElement,
                               YAML::Node &node) {
  node["filepath"] = videoElement->getVideoFilePath().string();
  node["framerate"] = videoElement->getFrameRate();
}

#pragma endregion

#pragma region RTMPStreamElement

RTMPStreamElement::RTMPStreamElement(std::string_view uid,
                                     RTMPServer &rtmpServer,
                                     std::string_view streamName, int frameRate)
    : Element(uid, frameRate), m_rtmpServer(rtmpServer),
      m_streamName(streamName) {
  reset();
}

void RTMPStreamElement::setStreamName(std::string_view streamName) {
  m_streamName = streamName;
}

bool RTMPStreamElement::nextFrame(cv::Mat &output) {
  return m_rtmpServer.receiveStreamFrame(m_streamName, output);
}

void RTMPStreamElement::reset() { setFrame(kNoFrameMat); }

static std::shared_ptr<Element> RTMPElementFromYAML(const std::string &uid,
                                                    RTMPServer &rtmpServer,
                                                    YAML::Node node) {
  const auto streamName = node["stream-name"].as<std::string>();
  const auto framerate = node["framerate"].as<int>();
  return std::make_shared<RTMPStreamElement>(uid, rtmpServer, streamName,
                                             framerate);
}

static void RTMPElementToYAML(std::shared_ptr<RTMPStreamElement> rtmpElement,
                              YAML::Node &node) {
  node["stream-name"] = rtmpElement->getStreamName();
  node["framerate"] = rtmpElement->getFrameRate();
}

#pragma endregion

#pragma region WebBrowserElement

WebBrowserElement::WebBrowserElement(std::string_view uid, std::string_view url,
                                     int frameRate, cv::Size viewSize)
    : Element(uid, frameRate), m_url(url), m_viewSize(viewSize),
      m_webBrowser(url, m_viewSize.width, m_viewSize.height) {
  reset();
}

void WebBrowserElement::setURL(std::string_view url) {
  m_url = url;
  m_webBrowser.loadURL(url);
}

void WebBrowserElement::setViewSize(cv::Size viewSize) {
  m_viewSize = viewSize;
  m_webBrowser.setViewSize(viewSize.width, viewSize.height);
}

bool WebBrowserElement::nextFrame(cv::Mat &output) {
  return m_webBrowser.getLatestFrame(output);
}

void WebBrowserElement::reset() { setFrame(kNoFrameMat); }

void WebBrowserElement::setCookie(
    std::string_view name, std::string_view value, std::string_view domain,
    std::string_view path, bool secure /*= false*/, bool httpOnly /*= false*/,
    cef_cookie_same_site_t sameSite /*= CEF_COOKIE_SAME_SITE_UNSPECIFIED*/) {

  CefCookie cookie;
  CefString(&cookie.name).FromString(name);
  CefString(&cookie.value).FromString(value);
  CefString(&cookie.domain).FromString(domain);
  CefString(&cookie.path).FromString(path);
  cookie.secure = secure;
  cookie.httponly = httpOnly;
  cookie.same_site = sameSite;
  cookie.has_expires = false; // Add expiration configuration in the future?

  m_webBrowser.setCookie(cookie);
  m_cookies.push_back(cookie);
}

static std::shared_ptr<Element>
WebBrowserElementFromYAML(const std::string &uid, YAML::Node node) {
  const auto url = node["url"].as<std::string>();
  const auto framerate = node["framerate"].as<int>();
  const auto viewSize = node["view-size"].as<cv::Size>();
  auto elem =
      std::make_shared<WebBrowserElement>(uid, url, framerate, viewSize);

  if (node["cookies"]) {
    for (const auto &cookie : node["cookies"]) {
      const auto name = cookie["name"].as<std::string>();
      const auto value = cookie["value"].as<std::string>();
      const auto domain = cookie["domain"].as<std::string>();
      std::string path = "/";
      if (cookie["path"]) {
        path = cookie["path"].as<std::string>();
      }
      bool secure = false;
      if (cookie["secure"]) {
        secure = cookie["secure"].as<bool>();
      }
      bool httpOnly = false;
      if (cookie["httpOnly"]) {
        httpOnly = cookie["httpOnly"].as<bool>();
      }
      cef_cookie_same_site_t sameSite = CEF_COOKIE_SAME_SITE_UNSPECIFIED;
      if (cookie["sameSite"]) {
        const auto sameSiteStr = cookie["sameSite"].as<std::string>();
        if (sameSiteStr == "None") {
          sameSite = CEF_COOKIE_SAME_SITE_NO_RESTRICTION;
        } else if (sameSiteStr == "Lax") {
          sameSite = CEF_COOKIE_SAME_SITE_LAX_MODE;
        } else if (sameSiteStr == "Strict") {
          sameSite = CEF_COOKIE_SAME_SITE_STRICT_MODE;
        } else if (!sameSiteStr.empty()) {
          spdlog::error("[Element] {}: invalid sameSite value for cookie: `{}'",
                        uid, sameSiteStr);
        }
      }

      elem->setCookie(name, value, domain, path, secure, httpOnly, sameSite);
    }
  }

  return elem;
}

static void
WebBrowserElementToYAML(std::shared_ptr<WebBrowserElement> webBrowserElement,
                        YAML::Node &node) {

  node["url"] = webBrowserElement->getURL();
  node["framerate"] = webBrowserElement->getFrameRate();
  node["view-size"] = webBrowserElement->getViewSize();

  YAML::Node cookiesNode;
  for (const CefCookie &cookie : webBrowserElement->getCookies()) {
    YAML::Node cookieNode;
    cookieNode["name"] = CefString(&cookie.name).ToString();
    cookieNode["value"] = CefString(&cookie.value).ToString();
    cookieNode["domain"] = CefString(&cookie.domain).ToString();
    cookieNode["path"] = CefString(&cookie.path).ToString();
    cookieNode["secure"] = static_cast<bool>(cookie.secure);
    cookieNode["httpOnly"] = static_cast<bool>(cookie.httponly);

    switch (cookie.same_site) {
    case CEF_COOKIE_SAME_SITE_NO_RESTRICTION:
      cookieNode["sameSite"] = "None";
      break;
    case CEF_COOKIE_SAME_SITE_LAX_MODE:
      cookieNode["sameSite"] = "Lax";
      break;
    case CEF_COOKIE_SAME_SITE_STRICT_MODE:
      cookieNode["sameSite"] = "Strict";
      break;
    default:
      break;
    }

    cookiesNode.push_back(cookieNode);
  }

  if (!webBrowserElement->getCookies().empty()) {
    node["cookies"] = cookiesNode;
  }
}

#pragma endregion

#pragma region TextElement

TextElement::TextElement(std::string_view uid, std::string_view text,
                         const std::filesystem::path &fontPath, int fontSize,
                         cv::Scalar color)
    : Element(uid, -1), m_text(text), m_fontPath(fontPath),
      m_fontSize(fontSize), m_color(std::move(color)) {

  render();
}

void TextElement::setText(std::string_view text) {
  if (m_text != text) {
    m_text = text;
    render();
  }
}
void TextElement::setFont(const std::filesystem::path &fontPath) {
  if (fontPath != m_fontPath) {
    m_fontPath = fontPath;
    render();
  }
}

void TextElement::setFontSize(int fontSize) {
  if (m_fontSize != fontSize) {
    m_fontSize = fontSize;
    render();
  }
}
void TextElement::setColor(const cv::Scalar &color) {
  if (m_color != color) {
    m_color = color;
    render();
  }
}

void TextElement::render() {
  setFrame(
      FontManager::get().renderText(m_text, m_fontPath, m_fontSize, m_color));
}

static cv::Scalar hexColorToScalar(const std::string &hexColor) {
  if (hexColor.length() != 7 || hexColor[0] != '#') {
    throw std::runtime_error(std::format("invalid hex color `{}'", hexColor));
    return {0, 0, 0};
  }

  int r, g, b;
  sscanf(hexColor.c_str(), "#%02x%02x%02x", &r, &g, &b);

  return cv::Scalar(b, g, r);
}

static std::shared_ptr<Element> TextElementFromYAML(const std::string &uid,
                                                    YAML::Node &node) {
  const auto text = node["text"].as<std::string>();
  const auto fontPath = node["font-path"].as<std::string>();
  const auto fontSize = node["font-size"].as<int>();
  const auto hexColor = node["color"].as<std::string>();
  const cv::Scalar color = hexColorToScalar(hexColor);
  return std::make_shared<TextElement>(uid, text, fontPath, fontSize, color);
}

static void TextElementToYAML(std::shared_ptr<TextElement> textElement,
                              YAML::Node node) {
  node["text"] = textElement->getText();
  node["font-path"] = textElement->getFontPath().string();
  node["font-size"] = textElement->getFontSize();

  cv::Scalar color = textElement->getColor();
  int b = color.val[0];
  int g = color.val[1];
  int r = color.val[2];
  char colorBuffer[8];
  snprintf(colorBuffer, sizeof(colorBuffer), "#%02x%02x%02x", r, g, b);

  node["color"] = std::string(colorBuffer);
}

#pragma endregion

#pragma region VirtualCanvas

VirtualCanvas::VirtualCanvas(const cv::Size &size) : m_dim(size) {
  clearPixelMatrix();
  setGamma(1.0);
}

void VirtualCanvas::setGamma(double gamma) {
  if (gamma < 0.0) {
    return;
  }

  m_gamma = gamma;

  uchar *lutPtr = m_canvasLUT.ptr();
  for (int i = 0; i < 256; ++i) {
    lutPtr[i] = cv::saturate_cast<uchar>(pow(i / 255.0, gamma) * 255.0);
  }
}

static void CommonElementPropsFromYAML(YAML::Node node, Element &element) {
  if (node["name"]) {
    const auto name = node["name"].as<std::string>();
    element.setName(name);
  }

  const auto location = node["location"].as<cv::Point>();
  element.setLocation(location);

  if (node["rotation"]) {
    const auto rotation = node["rotation"].as<double>();
    element.setRotation(rotation);
  }

  if (node["size"]) {
    const auto size = node["size"].as<cv::Size>();
    element.setSize(size);
  }

  if (node["preserve-aspect-ratio"]) {
    const auto preserveAspectRatio = node["preserve-aspect-ratio"].as<bool>();
    element.setPreserveAspectRatio(preserveAspectRatio);
  }
}

static void CommonElementPropsToYAML(std::shared_ptr<Element> element,
                                     YAML::Node &node) {
  if (!element->getName().empty()) {
    node["name"] = element->getName();
  }

  node["location"] = element->getRawLocation();

  const double rotation = element->getRotation();
  if (rotation != 0.0) {
    node["rotation"] = rotation;
  }

  cv::Size size = element->getSize();
  if (!size.empty()) {
    node["size"] = size;
  }

  if (element->wasPreserveAspectRatioChanged()) {
    node["preserve-aspect-ratio"] = element->getPreserveAspectRatio();
  }
}

bool VirtualCanvas::loadElementConfig(const std::filesystem::path &path,
                                      RTMPServer &rtmpServer) {
  spdlog::info("[Canvas] loading config from `{}'", path.string());
  try {
    YAML::Node configNode = YAML::LoadFile(path);

    // Settings
    {

      YAML::Node settingsNode = configNode["settings"];

      const auto gamma = settingsNode["gamma"].as<double>();
      if (gamma < 0) {
        throw std::invalid_argument("invalid gamma value: " +
                                    std::to_string(gamma));
      }
      setGamma(gamma);
    }

    // Elements
    {
      YAML::Node elementsNode = configNode["elements"];

      std::map<int, std::shared_ptr<Element>> elements;

      for (YAML::const_iterator it = elementsNode.begin();
           it != elementsNode.end(); ++it) {
        const auto uid = it->first.as<std::string>();
        YAML::Node elementNode = it->second;

        const auto elementTypeString = elementNode["type"].as<std::string>();
        const auto elementOrder = elementNode["order"].as<int>();

        if (elements.contains(elementOrder)) {
          throw std::invalid_argument("multiple elements with same order");
        }

        std::shared_ptr<Element> element;

        if (elementTypeString == "image") {
          element = ImageElementFromYAML(uid, elementNode);
        } else if (elementTypeString == "carousel") {
          element = CarouselElementFromYAML(uid, elementNode);
        } else if (elementTypeString == "video") {
          element = VideoElementFromYAML(uid, elementNode);
        } else if (elementTypeString == "rtmp") {
          element = RTMPElementFromYAML(uid, rtmpServer, elementNode);
        } else if (elementTypeString == "web-browser") {
          element = WebBrowserElementFromYAML(uid, elementNode);
        } else if (elementTypeString == "text") {
          element = TextElementFromYAML(uid, elementNode);
        } else {
          throw std::invalid_argument("invalid element type '" +
                                      elementTypeString + "'");
        }
        CommonElementPropsFromYAML(elementNode, *element);

        elements[elementOrder] = element;
      }

      // Top element with highest order, bottom with lowest order.
      for (const auto &element : elements | std::views::values) {
        addElement(element);
      }
    }

  } catch (const std::exception &e) {
    spdlog::error("[Canvas] failed load config from yaml: {}", e.what());
    return false;
  }

  spdlog::info("[Canvas] successfully loaded config");

  return true;
}

void VirtualCanvas::saveElementConfig(const std::filesystem::path &path) const {
  YAML::Node configNode;

  // Settings
  {
    YAML::Node settingsNode;

    settingsNode["gamma"] = m_gamma;

    configNode["settings"] = settingsNode;
  }

  // Elements
  {
    YAML::Node elementsNode;

    int order = 0;
    for (const auto &element : std::views::reverse(m_elements)) {
      YAML::Node elementNode;

      elementNode["order"] = order++;

      if (auto imageElement =
              std::dynamic_pointer_cast<ImageElement>(element)) {
        elementNode["type"] = "image";
        ImageElementToYAML(imageElement, elementNode);
      } else if (auto carouselElement =
                     std::dynamic_pointer_cast<CarouselElement>(element)) {
        elementNode["type"] = "carousel";
        CarouselElementToYAML(carouselElement, elementNode);
      } else if (auto videoElement =
                     std::dynamic_pointer_cast<VideoElement>(element)) {
        elementNode["type"] = "video";
        VideoElementToYAML(videoElement, elementNode);
      } else if (auto rtmpElement =
                     std::dynamic_pointer_cast<RTMPStreamElement>(element)) {
        elementNode["type"] = "rtmp";
        RTMPElementToYAML(rtmpElement, elementNode);
      } else if (auto webBrowserElement =
                     std::dynamic_pointer_cast<WebBrowserElement>(element)) {
        elementNode["type"] = "web-browser";
        WebBrowserElementToYAML(webBrowserElement, elementNode);
      } else if (auto textElement =
                     std::dynamic_pointer_cast<TextElement>(element)) {
        elementNode["type"] = "text";
        TextElementToYAML(textElement, elementNode);
      } else {
        spdlog::error("[Canvas] cannot save element of unknown type to config");
        return;
      }

      CommonElementPropsToYAML(element, elementNode);

      elementsNode[element->getUID()] = elementNode;
    }

    configNode["elements"] = elementsNode;
  }

  std::ofstream ofs(path);
  if (ofs) {
    ofs << configNode;
    spdlog::info("[Canvas] saved config to `{}'", path.string());
  } else {
    spdlog::error("[Canvas] failed to save config to file");
  }
}

void VirtualCanvas::clearElements() {
  m_elements.clear();
  m_elementPositions.clear();
}

std::shared_ptr<Element> VirtualCanvas::getElement(const std::string &uid) {
  auto posIt = m_elementPositions.find(uid);
  if (posIt != m_elementPositions.end()) {
    return *posIt->second;
  }

  return nullptr;
}

std::shared_ptr<const Element>
VirtualCanvas::getElement(const std::string &uid) const {
  auto posIt = m_elementPositions.find(uid);
  if (posIt != m_elementPositions.end()) {
    return *posIt->second;
  }

  return nullptr;
}

bool VirtualCanvas::addElement(std::shared_ptr<Element> element) {
  if (!element)
    return false;

  const std::string &uid = element->getUID();
  if (m_elementPositions.contains(uid)) {
    spdlog::error("[Canvas] addElement: element `{}' already exists", uid);
    return false;
  }

  m_elements.push_front(element);
  m_elementPositions[uid] = m_elements.begin();

  spdlog::info("[Canvas] addElement: added element `{}' to top", uid);

  pushElementsToPixelMatrix();
  return true;
}

bool VirtualCanvas::removeElement(const std::string &uid) {
  auto posIt = m_elementPositions.find(uid);

  if (posIt == m_elementPositions.end()) {
    spdlog::error("[Canvas] removeElement: element `{}' not found", uid);
    return false;
  }

  m_elements.erase(posIt->second);
  m_elementPositions.erase(posIt);

  spdlog::info("[Canvas] removeElement: removed element `{}'", uid);

  pushElementsToPixelMatrix();
  return true;
}

bool VirtualCanvas::moveElementUp(const std::string &uid) {
  auto posIt = m_elementPositions.find(uid);

  if (posIt == m_elementPositions.end()) {
    spdlog::error("[Canvas] moveElementUp: element `{}' not found", uid);
    return false;
  }

  auto current = posIt->second;

  if (current == m_elements.begin()) {
    spdlog::info("[Canvas] moveElementUp: element `{}' already on top", uid);
    // Already top, return success because why not?
    return true;
  }

  m_elements.splice(std::prev(current), m_elements, current);
  spdlog::info("[Canvas] moveElementUp: moved up element `{}'", uid);

  pushElementsToPixelMatrix();
  return true;
}

bool VirtualCanvas::moveElementDown(const std::string &uid) {
  auto posIt = m_elementPositions.find(uid);

  if (posIt == m_elementPositions.end()) {
    spdlog::error("[Canvas] moveElementDown: element `{}' not found", uid);
    return false;
  }

  auto current = posIt->second;
  auto next = std::next(current);

  if (next == m_elements.end()) {
    spdlog::info("[Canvas] moveElementDown: element `{}' already on bottom",
                 uid);
    // Already bottom, return success because why not?
    return true;
  }

  m_elements.splice(std::next(next), m_elements, current);
  spdlog::info("[Canvas] moveElementDown: moved down element `{}'", uid);

  pushElementsToPixelMatrix();
  return true;
}

void VirtualCanvas::pushElementsToPixelMatrix() {
  clearPixelMatrix();

  for (const auto &elemPtr : std::views::reverse(m_elements)) {
    cv::Point loc = elemPtr->getLocation();

    // Gets the current frame of the element object referenced by elemPtr
    cv::Mat elemMat = elemPtr->getPixelMatrix().clone();

    cv::Size elemSize = elemMat.size();

    if (elemSize.width <= 0 || elemSize.height <= 0) {
      continue;
    }

    /*
    Overwite a region of interest with the image. If the image does not fit on
    the canvas, we derive a new size and crop the element to it before
    transferring it to the canvas.
    */

    if ((loc.x <= m_dim.width) && (loc.y <= m_dim.height)) {

      if (loc.x + elemSize.width > m_dim.width) {
        elemSize.width = m_dim.width - loc.x;
      }

      if (loc.y + elemSize.height > m_dim.height) {
        elemSize.height = m_dim.height - loc.y;
      }

      elemMat = elemMat(cv::Rect(0, 0, elemSize.width, elemSize.height));

      // Apply the gamma LUT here - OpenCV DOES support in place lutting
      cv::LUT(elemMat, m_canvasLUT, elemMat);

      overlayImage(elemMat, cv::Rect(loc, elemSize));
    } else {
      spdlog::warn("[Element] {}: out of bounds, not rendered", elemPtr->getUID(),
                   elemPtr->getName());
    }
  }
}

void VirtualCanvas::overlayImage(const cv::Mat &overlay, cv::Rect roi) {
  int offsetX = 0, offsetY = 0;
  if (roi.x < 0) {
    offsetX = -roi.x;
    roi.width -= offsetX;
    roi.x = 0;
  }
  if (roi.y < 0) {
    offsetY = -roi.y;
    roi.height -= offsetY;
    roi.y = 0;
  }

  roi.width = std::min(roi.width, m_dim.width - roi.x);
  roi.height = std::min(roi.height, m_dim.height - roi.y);

  if (overlay.channels() == 4) {
    // Overlay image manually going pixel by pixel.
    for (int y = roi.y; y < roi.y + roi.height; ++y) {
      auto *canvasPtr = m_pixelMatrix.ptr<uint8_t>(y, roi.x);
      const auto *overlayPtr =
          overlay.ptr<uint8_t>(y - roi.y + offsetY, offsetX);

      for (int x = 0; x < roi.width; ++x) {
        const uint8_t *in = overlayPtr + (x * 4);
        uint8_t *out = canvasPtr + (x * 3);

        const uint16_t alpha = in[3];
        if (alpha == 255) {
          out[0] = in[0];
          out[1] = in[1];
          out[2] = in[2];
        } else {
          // Blending with integer math, faster than using floating point
          // (src * alpha + dst * (255 - alpha)) / 255
          out[0] = static_cast<uint8_t>(
              (in[0] * alpha + out[0] * (255 - alpha)) >> 8);
          out[1] = static_cast<uint8_t>(
              (in[1] * alpha + out[1] * (255 - alpha)) >> 8);
          out[2] = static_cast<uint8_t>(
              (in[2] * alpha + out[2] * (255 - alpha)) >> 8);
        }
      }
    }
  } else {
    overlay.copyTo(m_pixelMatrix(roi));
  }
}

#pragma endregion