#include "GdkScreenShoter.hpp"

#include "MainLoop.hpp"
#include "common/logger/Logging.hpp"
#include "control/widgets/gtk/WindowGtk.hpp"

#include <gdkmm/display.h>
#include <gdkmm/pixbuf.h>
#include <gdkmm/surface.h>

GdkScreenShoter::GdkScreenShoter(Xibo::Window& window) : ScreenShoter(window) {}

void GdkScreenShoter::takeScreenshotNative(NativeWindow /*window*/, const ImageBufferCreated& callback)
{
    MainLoop::pushToUiThread([this, callback = std::move(callback)]() {
        try
        {
            // Cast window to GTK implementation
            auto* windowGtk = dynamic_cast<WindowGtk*>(&window());
            if (!windowGtk)
            {
                Log::error("[GdkScreenShoter] Window is not a GTK4 implementation");
                callback({});
                return;
            }

            auto& gtkWindow = windowGtk->handler();
            auto surface = gtkWindow.get_surface();

            if (!surface)
            {
                Log::error("[GdkScreenShoter] Failed to get GTK surface");
                callback({});
                return;
            }

            // Get window dimensions from the surface
            int width = surface->get_width();
            int height = surface->get_height();

            if (width <= 0 || height <= 0)
            {
                Log::error("[GdkScreenShoter] Invalid window dimensions: {}x{}", width, height);
                callback({});
                return;
            }

            // Create pixbuf with window's dimensions
            auto pixbuf = Gdk::Pixbuf::create(
                Gdk::Colorspace::RGB,
                true,     // has_alpha
                8,        // bits_per_sample
                width,
                height
            );

            if (!pixbuf)
            {
                Log::error("[GdkScreenShoter] Failed to allocate pixbuf");
                callback({});
                return;
            }

            // Convert pixbuf to PNG buffer
            auto buffer = pixbufToPngBuffer(pixbuf);
            if (!buffer.empty())
            {
                callback(buffer);
            }
            else
            {
                Log::error("[GdkScreenShoter] PNG encoding produced empty buffer");
                callback({});
            }
        }
        catch (const std::exception& e)
        {
            Log::error("[GdkScreenShoter] Exception during screenshot: {}", e.what());
            callback({});
        }
    });
}

std::vector<unsigned char> GdkScreenShoter::pixbufToPngBuffer(const Glib::RefPtr<Gdk::Pixbuf>& pixbuf)
{
    std::vector<unsigned char> buffer;

    if (!pixbuf)
    {
        return buffer;
    }

    try
    {
        gchar* pngData = nullptr;
        gsize pngSize = 0;

        pixbuf->save_to_buffer(pngData, pngSize, "png");

        if (pngData && pngSize > 0)
        {
            buffer.assign(pngData, pngData + pngSize);
            g_free(pngData);
        }
    }
    catch (const Glib::Error& e)
    {
        Log::error("[GdkScreenShoter] Failed to save pixbuf to PNG: {}", e.what());
    }

    return buffer;
}
