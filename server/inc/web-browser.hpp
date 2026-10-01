#pragma once

#include <cef_browser.h>
#include <mutex>
#include <opencv2/core.hpp>
#include <span>
#include <string_view>

class WebBrowser {
public:
  WebBrowser(std::string_view url, unsigned int width, unsigned int height);
  ~WebBrowser();

  void loadURL(std::string_view url);
  void
  setCookie(std::string_view name, std::string_view value,
            std::string_view domain, std::string_view path, bool secure = false,
            bool httpOnly = false,
            cef_cookie_same_site_t sameSite = CEF_COOKIE_SAME_SITE_UNSPECIFIED);

  bool getLatestFrame(cv::Mat &frame);

private:
  friend class WebBrowserClient;

  CefRect getViewRect() const { return m_viewRect; }
  void onPaint(std::span<const CefRect> dirtyRects, const void *buffer,
               int width, int height);

private:
  CefString m_url;

  CefWindowInfo m_windowInfo;
  CefBrowserSettings m_browserSettings;

  CefRefPtr<CefBrowser> m_browser;
  CefRect m_viewRect;

  bool m_hasFrame = false;
  cv::Mat m_latestFrame;
  std::mutex m_frameMutex;
};
