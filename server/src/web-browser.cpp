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
    nextWebBrowser = webBrowser;
  }

  void unregisterWebBrowser(WebBrowser *webBrowser) {
    webBrowsers.erase(webBrowser->browser->GetIdentifier());
  }

private:
  // CefClient methods.

  CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
  CefRefPtr<CefRenderHandler> GetRenderHandler() override { return this; }

  // CefLifeSpanHandler methods.

  void OnAfterCreated(CefRefPtr<CefBrowser> browser) override {
    CEF_REQUIRE_UI_THREAD();

    numBrowsers++;

    if (nextWebBrowser) {
      nextWebBrowser->browser = browser;
      webBrowsers[browser->GetIdentifier()] = nextWebBrowser;
      nextWebBrowser = nullptr;
    }
  }

  void OnBeforeClose(CefRefPtr<CefBrowser> browser) override {
    CEF_REQUIRE_UI_THREAD();

    webBrowsers.erase(browser->GetIdentifier());
    numBrowsers--;
  }

  // CefRenderHandler methods.

  void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) override {
    auto it = webBrowsers.find(browser->GetIdentifier());
    if (it != webBrowsers.end()) {
      rect = it->second->getViewRect();
    }
  }

  void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type,
               const RectList &dirtyRects, const void *buffer, int width,
               int height) override {
    auto it = webBrowsers.find(browser->GetIdentifier());
    if (it != webBrowsers.end()) {
      it->second->onPaint(dirtyRects, buffer, width, height);
    }
  }

  IMPLEMENT_REFCOUNTING(WebBrowserClient);

private:
  int numBrowsers = 0;
  std::unordered_map<int, WebBrowser *> webBrowsers;
  WebBrowser *nextWebBrowser = nullptr;
};

WebBrowser::WebBrowser(const std::string &_url, unsigned int width,
                       unsigned int height)
    : url(_url), viewRect(0, 0, width, height) {
  windowInfo.SetAsWindowless(0);

  WebBrowserClient::get()->setNextWebBrowser(this);

  browser =
      CefBrowserHost::CreateBrowserSync(windowInfo, WebBrowserClient::get(),
                                        url, browserSettings, nullptr, nullptr);
}

WebBrowser::~WebBrowser() {
  WebBrowserClient::get()->unregisterWebBrowser(this);
}

void WebBrowser::loadURL(const std::string &url) {
  browser->GetMainFrame()->LoadURL(CefString(url));
}

void WebBrowser::setCookie(const std::string &name, const std::string &value,
                           const std::string &domain, const std::string &path,
                            bool secure, bool httpOnly,
                            cef_cookie_same_site_t sameSite) {
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

  cookieManager->SetCookie(url, cookie, nullptr);

  cookieManager->FlushStore(nullptr);
}

bool WebBrowser::getLatestFrame(cv::Mat &frame) {
  std::lock_guard<std::mutex> lock(frameMutex);
  if (hasFrame) {
    frame = latestFrame.clone();
    return true;
  }
  return false;
}

void WebBrowser::onPaint(const std::span<const CefRect> dirtyRects,
                         const void *buffer, int width, int height) {

  std::lock_guard<std::mutex> lock(frameMutex);
  (void)dirtyRects;
  hasFrame = true;
  latestFrame = cv::Mat(height, width, CV_8UC4, (void *)buffer).clone();
}
