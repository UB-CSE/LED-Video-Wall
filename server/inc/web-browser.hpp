#ifndef WEB_BROWSER_HPP
#define WEB_BROWSER_HPP

#include <cef_browser.h>
#include <string>
#include <span>
#include <mutex>
#include <opencv2/core.hpp>

class WebBrowser {
public:
  WebBrowser(const std::string &url, unsigned int width, unsigned int height);
  ~WebBrowser();

  void loadURL(const std::string &url);

  bool getLatestFrame(cv::Mat &frame);

private:
  friend class WebBrowserClient;

  CefRect getViewRect() const { return viewRect; }
  void onPaint(const std::span<const CefRect> dirtyRects, const void *buffer, int width,
               int height);

private:
  CefWindowInfo windowInfo;
  CefBrowserSettings browserSettings;

  CefRefPtr<CefBrowser> browser;
  CefRect viewRect;

  bool hasFrame = false;
  cv::Mat latestFrame;
  std::mutex frameMutex;
};

#endif // WEB_BROWSER_HPP
