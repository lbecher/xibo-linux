#include "ScreenShoter.hpp"

#include <boost/beast/core/detail/base64.hpp>

ScreenShoter::ScreenShoter(Xibo::Window& window) : window_(window) {}

void ScreenShoter::takeBase64(const ScreenShotTaken& callback)
{
    takeScreenshotNative(nativeWindow(), [callback = std::move(callback)](const ImageBuffer& buffer) {
        std::string base64;
        base64.resize(boost::beast::detail::base64::encoded_size(buffer.size()));
        boost::beast::detail::base64::encode(&base64[0], buffer.data(), buffer.size());
        callback(base64);
    });
}

NativeWindow ScreenShoter::nativeWindow() const
{
    return window_.nativeWindow();
}
