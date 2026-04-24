#pragma once

#include "control/screenshot/PortalScreenShoter.hpp"
#include "control/screenshot/ScreenShoter.hpp"

namespace ScreenShoterFactory
{
    /**
     * @brief Screenshot factory for compositor-safe capture
     *
     * We currently route every capture through the desktop portal because the
     * GTK screenshot implementation only allocates an empty pixbuf and encodes
     * it, which produces black images instead of a real window capture.
     *
     * Portals are the supported path on Wayland compositors such as Sway and
     * are also a safe fallback on other desktops.
     */
    inline std::unique_ptr<ScreenShoter> create(Xibo::Window& window)
    {
        return std::make_unique<PortalScreenShoter>(window);
    }
}
