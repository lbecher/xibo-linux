#include "WebViewFactory.hpp"

#include "control/media/webview/WebView.hpp"
#include "control/media/webview/WebViewWidgetFactory.hpp"

#include "control/media/MediaImpl.hpp"
#include "control/media/MediaResources.hpp"

std::unique_ptr<Xibo::Media> WebViewFactory::create(const MediaOptions& options,
                                                    int width,
                                                    int height,
                                                    bool transparency)
{
    auto media = std::make_unique<MediaImpl>(options);
    media->setWidget(createView(options.uri, width, height, static_cast<Xibo::WebView::Transparency>(transparency)));
    return media;
}

std::shared_ptr<Xibo::WebView> WebViewFactory::createView(const Uri& uri,
                                                          int width,
                                                          int height,
                                                          Xibo::WebView::Transparency transparency)
{
    auto webview = WebViewWidgetFactory::create(width, height);

    webview->load(uri);
    if (transparency == Xibo::WebView::Transparency::Enable)
    {
        webview->enableTransparency();
    }

    return webview;
}

void WebViewFactory::updateViewPortWidth(const Uri& uri, int width)
{
    (void)uri;
    (void)width;
}
