#ifndef WEB_BROWSER_HPP
#define WEB_BROWSER_HPP

#include <cef_browser.h>
#include <mutex>
#include <opencv2/core.hpp>
#include <span>
#include <string>

class WebBrowser {
public:
  WebBrowser(const std::string &url, unsigned int width, unsigned int height);
  ~WebBrowser();

  void loadURL(const std::string &url);
  void
  setCookie(const std::string &name, const std::string &value,
            const std::string &domain, const std::string &path,
            bool secure = false, bool httpOnly = false,
            cef_cookie_same_site_t sameSite = CEF_COOKIE_SAME_SITE_UNSPECIFIED);

  bool getLatestFrame(cv::Mat &frame);

private:
  friend class WebBrowserClient;

  CefRect getViewRect() const { return viewRect; }
  void onPaint(const std::span<const CefRect> dirtyRects, const void *buffer,
               int width, int height);

private:
  CefString url;

  CefWindowInfo windowInfo;
  CefBrowserSettings browserSettings;

  CefRefPtr<CefBrowser> browser;
  CefRect viewRect;

  bool hasFrame = false;
  cv::Mat latestFrame;
  std::mutex frameMutex;
};

#endif // WEB_BROWSER_HPP
