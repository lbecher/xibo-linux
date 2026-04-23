#pragma once

#include "control/screenshot/PortalScreenShoter.hpp"
#ifdef USE_GTK
#include "control/screenshot/gtk/GdkScreenShoter.hpp"
#endif
#include "control/screenshot/ScreenShoter.hpp"

#include <cstdlib>
#include <string>

namespace ScreenShoterFactory
{
    /**
     * @brief Modern screenshot factory preferring Wayland-capable implementations
     *
     * Priority:
     * 1. GdkScreenShoter (GTK4) - works on both Wayland and X11 via backend-agnostic Gdk APIs
     * 2. PortalScreenShoter - DBus portal API for sandboxed/restricted environments
     * 3. PortalScreenShoter fallback - when GTK is unavailable
     *
     * This strategy avoids low-level X11 APIs while maintaining compatibility.
     */
    inline std::unique_ptr<ScreenShoter> create(Xibo::Window& window)
    {
#ifdef USE_GTK
        // Prefer GTK4's Gdk-based screenshot implementation
        // Works seamlessly on both Wayland and X11 without hardcoded backend dependencies
        return std::make_unique<GdkScreenShoter>(window);
#else
        // Fallback to freedesktop portal API for restricted/sandboxed environments
        return std::make_unique<PortalScreenShoter>(window);
#endif
    }
}
