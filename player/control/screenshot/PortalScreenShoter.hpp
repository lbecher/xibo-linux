#pragma once

#include "control/screenshot/ScreenShoter.hpp"

#include <string>

struct DBusConnection;
struct DBusError;
struct DBusMessage;

class PortalScreenShoter : public ScreenShoter
{
public:
    PortalScreenShoter(Xibo::Window& window);

private:
    void takeScreenshotNative(NativeWindow window, const ImageBufferCreated& callback) override;

    ImageBuffer takeScreenshot();
    DBusConnection* connectToSessionBus(DBusError& error);
    DBusMessage* createScreenshotRequest();
    std::string sendRequest(DBusConnection* connection, DBusMessage* message, DBusError& error);
    std::string waitForResponse(DBusConnection* connection, const std::string& handlePath, DBusError& error);
    std::string uriFromResponse(DBusMessage* message);
    ImageBuffer readFileUri(const std::string& uri);
    std::string filePathFromUri(const std::string& uri);
};
