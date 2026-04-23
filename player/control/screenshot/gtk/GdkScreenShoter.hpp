#pragma once

#include "control/screenshot/ScreenShoter.hpp"

#include <gdkmm/pixbuf.h>

/**
 * @brief Modern GTK4 screenshot implementation using GdkPixbuf
 *
 * This implementation is backend-agnostic and works with both Wayland and X11.
 * It uses the GTK/Gdk rendering pipeline to capture screenshots without
 * depending on low-level X11 APIs, making it the preferred approach for
 * modern GTK4 applications.
 */
class GdkScreenShoter : public ScreenShoter
{
public:
    explicit GdkScreenShoter(Xibo::Window& window);

protected:
    void takeScreenshotNative(NativeWindow window, const ImageBufferCreated& callback) override;

private:
    std::vector<unsigned char> pixbufToPngBuffer(const Glib::RefPtr<Gdk::Pixbuf>& pixbuf);
};
