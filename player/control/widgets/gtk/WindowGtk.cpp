#include "WindowGtk.hpp"

#include "common/logger/Logging.hpp"

#include <boost/format.hpp>
#include <gdk/gdk.h>
#include <gdkmm/monitor.h>
#include <gtkmm/cssprovider.h>
#include <gtkmm/eventcontrollerkey.h>
#include <gtkmm/styleprovider.h>

WindowGtk::WindowGtk() : SingleContainerGtk{handler_}, cursorVisible_(true), fullscreen_(false)
{
    handler_.signal_realize().connect(std::bind(&WindowGtk::onWindowRealized, this));

    auto keyController = Gtk::EventControllerKey::create();
    keyController->signal_key_pressed().connect(
        [this](guint keyval, guint /*keycode*/, Gdk::ModifierType /*state*/) {
            const char* keyName = gdk_keyval_name(keyval);
            keyPressed_(KeyboardKey{keyName ? keyName : "", keyval});
            return false;
        },
        false);
    handler_.add_controller(keyController);

    handler_.property_fullscreened().signal_changed().connect([this]() { fullscreen_ = handler_.is_fullscreen(); });
}

void WindowGtk::addToHandler(const std::shared_ptr<Xibo::Widget>& child)
{
    handler_.set_child(handlerFor(child));
}

void WindowGtk::removeFromHandler(const std::shared_ptr<Xibo::Widget>& child)
{
    if (handler_.get_child() == &handlerFor(child))
    {
        handler_.unset_child();
    }
}

void WindowGtk::setSize(int width, int height)
{
    resizeWindow(width, height);
}

void WindowGtk::resizeWindow(int width, int height)
{
    SingleContainerGtk::setSize(width, height);
    handler_.set_default_size(width, height);
}

int WindowGtk::x() const
{
    return x_;
}

int WindowGtk::y() const
{
    return y_;
}

void WindowGtk::move(int x, int y)
{
    x_ = x;
    y_ = y;
}

void WindowGtk::disableWindowResize()
{
    handler_.set_resizable(false);
}

void WindowGtk::disableWindowDecoration()
{
    handler_.set_decorated(false);
}

SignalKeyPressed& WindowGtk::keyPressed()
{
    return keyPressed_;
}

void WindowGtk::fullscreen()
{
    auto monitorGeometry = currentMonitorGeometry();
    if (!monitorGeometry.has_zero_area())
    {
        // Fullscreen means that window will be shown on monitor dimensions withour borders but children are not
        // automatically resized during this operation so we need to force them to update their sizes.
        // Also, we need to save previous window dimensions to restore them later
        resizeWindow(monitorGeometry.get_width(), monitorGeometry.get_height());

        fullscreen_ = true;
        handler_.fullscreen();  // TODO: probably use fullscreen on specific monitor
    }
    else
    {
        Log::error("[WindowGtk] Failed to get current monitor geometry");
    }
}

void WindowGtk::unfullscreen()
{
    if (isFullscreen())
    {
        fullscreen_ = false;
        handler_.unfullscreen();
    }
}

bool WindowGtk::isFullscreen() const
{
    return fullscreen_;
}

void WindowGtk::setCursorVisible(bool cursorVisible)
{
    cursorVisible_ = cursorVisible;
    updateCursor();
}

void WindowGtk::onWindowRealized()
{
    updateCursor();
}

Gdk::Rectangle WindowGtk::currentMonitorGeometry()
{
    Gdk::Rectangle geometry;
    auto surface = handler_.get_surface();
    auto display = handler_.get_display();

    if (surface && display)
    {
        if (auto monitor = display->get_monitor_at_surface(surface))
        {
            monitor->get_geometry(geometry);
        }
    }

    if (geometry.has_zero_area() && display)
    {
        auto* monitors = gdk_display_get_monitors(display->gobj());
        if (monitors && g_list_model_get_n_items(monitors) > 0)
        {
            auto* monitor = GDK_MONITOR(g_list_model_get_item(monitors, 0));
            GdkRectangle monitorGeometry;
            gdk_monitor_get_geometry(monitor, &monitorGeometry);
            geometry.set_x(monitorGeometry.x);
            geometry.set_y(monitorGeometry.y);
            geometry.set_width(monitorGeometry.width);
            geometry.set_height(monitorGeometry.height);
            g_object_unref(monitor);
        }
    }

    return geometry;
}

void WindowGtk::updateCursor()
{
    handler_.set_cursor(cursorVisible_ ? "" : "none");
}

void WindowGtk::setBackgroundColor(const Color& color)
{
    boost::format windowStyleFmt{"window { background-color: %1%; }"};

    auto cssProvider = Gtk::CssProvider::create();
    auto display = handler_.get_display();
    auto windowStyle = (windowStyleFmt % color.string()).str();

    if (cssProvider && display)
    {
        cssProvider->load_from_data(windowStyle);
        Gtk::StyleProvider::add_provider_for_display(display, cssProvider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
    else
    {
        Log::error("[WindowGtk] Failed to set background color");
    }
}

NativeWindow WindowGtk::nativeWindow()
{
    // Modern GTK4/Wayland approach: use screenshot abstraction instead of native window ID
    // This method is deprecated in favor of backend-agnostic screenshot implementation
    return DefaultNativeWindow;
}

void WindowGtk::setKeepAbove(bool keep_above)
{
    (void)keep_above;
    Log::debug("[WindowGtk] setKeepAbove is not supported by GTK4");
}

Gtk::Window& WindowGtk::handler()
{
    return handler_;
}
