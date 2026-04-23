#include "WebViewGtk.hpp"

#include "common/PlayerRuntimeError.hpp"
#include "common/logger/Logging.hpp"
#include "common/types/Uri.hpp"

#ifdef USE_WEBKITGTK
#include <webkit/webkit.h>
#endif

namespace ph = std::placeholders;

WebViewGtk::WebViewGtk(int width, int height) : WidgetGtk{handler_}
{
    WidgetGtk::setSize(width, height);

#ifdef USE_WEBKITGTK
    auto* webView = webkit_web_view_new();
    webViewWidget_ = Glib::wrap(webView);
    webViewWidget_->set_hexpand(true);
    webViewWidget_->set_vexpand(true);
    webViewWidget_->set_size_request(width, height);
    webViewWidget_->set_visible(true);
    handler_.set_child(*webViewWidget_);
    handler_.set_policy(Gtk::PolicyType::NEVER, Gtk::PolicyType::NEVER);

    g_signal_connect(webView, "load-failed", G_CALLBACK(+[](WebKitWebView*,
                                                            WebKitLoadEvent,
                                                            char* failingUri,
                                                            GError* error,
                                                            gpointer) -> gboolean {
                         Log::error("[WebViewGtk] Failed to load {}: {}",
                                    failingUri ? failingUri : "",
                                    error ? error->message : "unknown error");
                         return false;
                     }),
                     nullptr);

    g_signal_connect(webView, "load-changed", G_CALLBACK(+[](WebKitWebView*, WebKitLoadEvent loadEvent, gpointer) {
                         if (loadEvent == WEBKIT_LOAD_FINISHED)
                         {
                             Log::debug("[WebViewGtk] Load finished");
                         }
                     }),
                     nullptr);
#endif
}

void WebViewGtk::show()
{
    handler_.set_visible(true);
}

void WebViewGtk::setSize(int width, int height)
{
    WidgetGtk::setSize(width, height);
#ifdef USE_WEBKITGTK
    if (webViewWidget_)
    {
        webViewWidget_->set_size_request(width, height);
    }
#endif
}

void WebViewGtk::reload()
{
#ifdef USE_WEBKITGTK
    webkit_web_view_reload(WEBKIT_WEB_VIEW(webViewWidget_->gobj()));
#else
    throw PlayerRuntimeError{"WebViewGtk", "WebKitGTK support is not available"};
#endif
}

void WebViewGtk::load(const Uri& uri)
{
#ifdef USE_WEBKITGTK
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webViewWidget_->gobj()), uri.string().c_str());
#else
    (void)uri;
    throw PlayerRuntimeError{"WebViewGtk", "WebKitGTK support is not available"};
#endif
}

void WebViewGtk::enableTransparency()
{
#ifdef USE_WEBKITGTK
    GdkRGBA transparent;
    transparent.red = 0.0;
    transparent.green = 0.0;
    transparent.blue = 0.0;
    transparent.alpha = 0.0;
    webkit_web_view_set_background_color(WEBKIT_WEB_VIEW(webViewWidget_->gobj()), &transparent);
#endif
}

Gtk::ScrolledWindow& WebViewGtk::handler()
{
    return handler_;
}
