#pragma once

#include "control/screenshot/PortalScreenShoter.hpp"
#ifdef USE_GTK
#include "control/screenshot/gtk/X11ScreenShoter.hpp"
#endif
#include "control/screenshot/ScreenShoter.hpp"

#include <cstdlib>
#include <string>

namespace ScreenShoterFactory
{
    inline bool isWaylandSession()
    {
        const char* sessionType = std::getenv("XDG_SESSION_TYPE");
        const char* waylandDisplay = std::getenv("WAYLAND_DISPLAY");

        return (sessionType && std::string{sessionType} == "wayland") ||
               (waylandDisplay && std::string{waylandDisplay}.size() > 0);
    }

    inline std::unique_ptr<ScreenShoter> create(Xibo::Window& window)
    {
        if (isWaylandSession())
        {
            return std::make_unique<PortalScreenShoter>(window);
        }

#ifdef USE_GTK
        return std::make_unique<X11ScreenShoter>(window);
#else
        return std::make_unique<PortalScreenShoter>(window);
#endif
    }
}
