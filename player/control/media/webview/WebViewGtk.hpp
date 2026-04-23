#pragma once

#include "control/media/webview/WebView.hpp"
#include "control/widgets/gtk/WidgetGtk.hpp"

#include <gtkmm/scrolledwindow.h>

class Uri;

class WebViewGtk : public WidgetGtk<Xibo::WebView>
{
public:
    WebViewGtk(int width, int height);

    void show() override;
    void setSize(int width, int height) override;

    void reload() override;
    void load(const Uri& uri) override;
    void enableTransparency() override;

    Gtk::ScrolledWindow& handler() override;

private:
    Gtk::ScrolledWindow handler_;
    Gtk::Widget* webViewWidget_ = nullptr;
    sigc::connection sizeAllocateConnection_;
};
