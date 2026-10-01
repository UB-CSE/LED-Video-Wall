#include "web-browser.hpp"

#include <cef_client.h>
#include <cef_life_span_handler.h>
#include <cef_render_handler.h>
#include <unordered_map>
#include <wrapper/cef_helpers.h>

class WebBrowserClient : public CefClient,
                         public CefLifeSpanHandler,
                         public CefRenderHandler {

public:
  static WebBrowserClient *get() {
    static WebBrowserClient instance;
    return &instance;
  }

  void setNextWebBrowser(WebBrowser *webBrowser) {
    m_nextWebBrowser = webBrowser;
  }

  void unregisterWebBrowser(const WebBrowser *webBrowser) {
    m_webBrowsers.erase(webBrowser->m_browser->GetIdentifier());
  }

private:
  // CefClient methods.

  CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
  CefRefPtr<CefRenderHandler> GetRenderHandler() override { return this; }

  // CefLifeSpanHandler methods.

  void OnAfterCreated(CefRefPtr<CefBrowser> browser) override {
    CEF_REQUIRE_UI_THREAD();

    m_numBrowsers++;

    if (m_nextWebBrowser) {
      m_nextWebBrowser->m_browser = browser;
      m_webBrowsers[browser->GetIdentifier()] = m_nextWebBrowser;
      m_nextWebBrowser = nullptr;
    }
  }

  void OnBeforeClose(CefRefPtr<CefBrowser> browser) override {
    CEF_REQUIRE_UI_THREAD();

    m_webBrowsers.erase(browser->GetIdentifier());
    m_numBrowsers--;
  }

  // CefRenderHandler methods.

  void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) override {
    auto it = m_webBrowsers.find(browser->GetIdentifier());
    if (it != m_webBrowsers.end()) {
      rect = it->second->getViewRect();
    }
  }

  void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type,
               const RectList &dirtyRects, const void *buffer, int width,
               int height) override {
    auto it = m_webBrowsers.find(browser->GetIdentifier());
    if (it != m_webBrowsers.end()) {
      it->second->onPaint(dirtyRects, buffer, width, height);
    }
  }

  IMPLEMENT_REFCOUNTING(WebBrowserClient);

private:
  int m_numBrowsers = 0;
  std::unordered_map<int, WebBrowser *> m_webBrowsers;
  WebBrowser *m_nextWebBrowser = nullptr;
};

WebBrowser::WebBrowser(std::string_view url, unsigned int width,
                       unsigned int height)
    : m_url(std::string(url)),
      m_viewRect(0, 0, static_cast<int>(width), static_cast<int>(height)) {
  m_windowInfo.SetAsWindowless(0);

  WebBrowserClient::get()->setNextWebBrowser(this);

  m_browser = CefBrowserHost::CreateBrowserSync(
      m_windowInfo, WebBrowserClient::get(), m_url, m_browserSettings, nullptr,
      nullptr);
}

WebBrowser::~WebBrowser() {
  WebBrowserClient::get()->unregisterWebBrowser(this);
}

void WebBrowser::loadURL(std::string_view url) {
  m_browser->GetMainFrame()->LoadURL(CefString(std::string(url)));
}

void WebBrowser::setCookie(
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

  CefRefPtr<CefCookieManager> cookieManager =
      CefCookieManager::GetGlobalManager(nullptr);

  cookieManager->SetCookie(m_url, cookie, nullptr);

  cookieManager->FlushStore(nullptr);
}

bool WebBrowser::getLatestFrame(cv::Mat &frame) {
  std::lock_guard lock(m_frameMutex);
  if (m_hasFrame) {
    frame = m_latestFrame.clone();
    return true;
  }
  return false;
}

void WebBrowser::onPaint([[maybe_unused]] std::span<const CefRect> dirtyRects,
                         const void *buffer, int width, int height) {

  std::lock_guard lock(m_frameMutex);
  m_hasFrame = true;
  m_latestFrame =
      cv::Mat(height, width, CV_8UC4, const_cast<void *>(buffer)).clone();
}
