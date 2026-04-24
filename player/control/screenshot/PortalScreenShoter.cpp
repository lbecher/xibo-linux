#include "PortalScreenShoter.hpp"

#include "common/logger/Logging.hpp"

#include <dbus/dbus.h>

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <thread>

namespace
{
    constexpr const char* PortalBus = "org.freedesktop.portal.Desktop";
    constexpr const char* PortalPath = "/org/freedesktop/portal/desktop";
    constexpr const char* ScreenshotInterface = "org.freedesktop.portal.Screenshot";
    constexpr const char* RequestInterface = "org.freedesktop.portal.Request";
    constexpr const char* ResponseMember = "Response";
    constexpr const int PortalTimeoutMs = 300000;
    constexpr const char* InteractiveEnvVar = "XIBO_SCREENSHOT_PORTAL_INTERACTIVE";

    bool appendOption(DBusMessageIter& options, const char* key, const char* signature, const void* value)
    {
        DBusMessageIter dictEntry;
        DBusMessageIter variant;

        if (!dbus_message_iter_open_container(&options, DBUS_TYPE_DICT_ENTRY, nullptr, &dictEntry)) return false;
        if (!dbus_message_iter_append_basic(&dictEntry, DBUS_TYPE_STRING, &key)) return false;
        if (!dbus_message_iter_open_container(&dictEntry, DBUS_TYPE_VARIANT, signature, &variant)) return false;

        int type = signature[0];
        if (!dbus_message_iter_append_basic(&variant, type, value)) return false;
        if (!dbus_message_iter_close_container(&dictEntry, &variant)) return false;
        return dbus_message_iter_close_container(&options, &dictEntry);
    }

    unsigned char hexToByte(char high, char low)
    {
        auto valueOf = [](char c) -> unsigned char {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return 0;
        };

        return static_cast<unsigned char>((valueOf(high) << 4) | valueOf(low));
    }

    bool envVarEnabled(const char* name)
    {
        auto* value = std::getenv(name);
        if (!value) return false;

        std::string normalized{value};
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        return normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on";
    }

    std::string portalResponseMeaning(uint32_t response)
    {
        switch (response)
        {
            case 0:
                return "success";
            case 1:
                return "cancelled";
            case 2:
                return "denied";
            default:
                return "unknown";
        }
    }

    std::string resultKeysSummary(DBusMessageIter& results)
    {
        std::ostringstream summary;
        bool first = true;

        while (dbus_message_iter_get_arg_type(&results) == DBUS_TYPE_DICT_ENTRY)
        {
            DBusMessageIter entry;
            const char* key = nullptr;

            dbus_message_iter_recurse(&results, &entry);
            if (dbus_message_iter_get_arg_type(&entry) == DBUS_TYPE_STRING)
            {
                dbus_message_iter_get_basic(&entry, &key);
                if (key)
                {
                    if (!first) summary << ",";
                    summary << key;
                    first = false;
                }
            }

            dbus_message_iter_next(&results);
        }

        return summary.str();
    }
}

PortalScreenShoter::PortalScreenShoter(Xibo::Window& window) : ScreenShoter(window) {}

void PortalScreenShoter::takeScreenshotNative(NativeWindow /*window*/, const ImageBufferCreated& callback)
{
    std::thread([this, callback]() {
        try
        {
            callback(takeScreenshot());
        }
        catch (const std::exception& e)
        {
            Log::error("[PortalScreenShoter] {}", e.what());
            callback({});
        }
    }).detach();
}

ImageBuffer PortalScreenShoter::takeScreenshot()
{
    DBusError error;
    dbus_error_init(&error);

    DBusConnection* connection = nullptr;
    DBusMessage* message = nullptr;

    try
    {
        connection = connectToSessionBus(error);
        message = createScreenshotRequest();
        auto handlePath = sendRequest(connection, message, error);
        auto uri = waitForResponse(connection, handlePath, error);

        dbus_message_unref(message);
        dbus_connection_unref(connection);

        return readFileUri(uri);
    }
    catch (...)
    {
        if (message) dbus_message_unref(message);
        if (connection) dbus_connection_unref(connection);
        if (dbus_error_is_set(&error)) dbus_error_free(&error);
        throw;
    }
}

DBusConnection* PortalScreenShoter::connectToSessionBus(DBusError& error)
{
    auto connection = dbus_bus_get(DBUS_BUS_SESSION, &error);
    if (!connection)
    {
        throw PlayerRuntimeError{"PortalScreenShoter", error.message ? error.message : "Unable to connect to DBus"};
    }

    return connection;
}

DBusMessage* PortalScreenShoter::createScreenshotRequest()
{
    auto message = dbus_message_new_method_call(PortalBus, PortalPath, ScreenshotInterface, "Screenshot");
    if (!message) throw PlayerRuntimeError{"PortalScreenShoter", "Unable to create portal request"};

    DBusMessageIter args;
    DBusMessageIter options;
    const char* parentWindow = "";
    dbus_bool_t interactive = envVarEnabled(InteractiveEnvVar);

    Log::info("[PortalScreenShoter] Requesting screenshot via portal (interactive={}, parent_window=<empty>)",
              interactive ? "true" : "false");

    dbus_message_iter_init_append(message, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &parentWindow))
    {
        dbus_message_unref(message);
        throw PlayerRuntimeError{"PortalScreenShoter", "Unable to append parent window"};
    }

    if (!dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, "{sv}", &options) ||
        !appendOption(options, "interactive", DBUS_TYPE_BOOLEAN_AS_STRING, &interactive) ||
        !dbus_message_iter_close_container(&args, &options))
    {
        dbus_message_unref(message);
        throw PlayerRuntimeError{"PortalScreenShoter", "Unable to append portal options"};
    }

    return message;
}

std::string PortalScreenShoter::sendRequest(DBusConnection* connection, DBusMessage* message, DBusError& error)
{
    auto reply = dbus_connection_send_with_reply_and_block(connection, message, DBUS_TIMEOUT_USE_DEFAULT, &error);
    if (!reply)
    {
        throw PlayerRuntimeError{"PortalScreenShoter", error.message ? error.message : "Portal request failed"};
    }

    const char* handlePath = nullptr;
    bool success = dbus_message_get_args(reply, &error, DBUS_TYPE_OBJECT_PATH, &handlePath, DBUS_TYPE_INVALID);
    std::string result = handlePath ? handlePath : "";
    dbus_message_unref(reply);

    if (!success || result.empty())
    {
        throw PlayerRuntimeError{"PortalScreenShoter", error.message ? error.message : "Portal did not return a handle"};
    }

    return result;
}

std::string PortalScreenShoter::waitForResponse(DBusConnection* connection,
                                                const std::string& handlePath,
                                                DBusError& error)
{
    std::string matchRule = "type='signal',sender='" + std::string(PortalBus) + "',path='" + handlePath +
                            "',interface='" + RequestInterface + "',member='" + ResponseMember + "'";
    dbus_bus_add_match(connection, matchRule.c_str(), &error);
    dbus_connection_flush(connection);

    if (dbus_error_is_set(&error))
    {
        throw PlayerRuntimeError{"PortalScreenShoter", error.message ? error.message : "Unable to listen for portal"};
    }

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(PortalTimeoutMs);
    while (std::chrono::steady_clock::now() < deadline)
    {
        dbus_connection_read_write(connection, 250);
        DBusMessage* message = dbus_connection_pop_message(connection);
        if (!message) continue;

        if (dbus_message_is_signal(message, RequestInterface, ResponseMember) &&
            handlePath == dbus_message_get_path(message))
        {
            auto uri = uriFromResponse(message);
            dbus_message_unref(message);
            dbus_bus_remove_match(connection, matchRule.c_str(), nullptr);
            return uri;
        }

        dbus_message_unref(message);
    }

    dbus_bus_remove_match(connection, matchRule.c_str(), nullptr);
    throw PlayerRuntimeError{"PortalScreenShoter", "Timed out waiting for portal response"};
}

std::string PortalScreenShoter::uriFromResponse(DBusMessage* message)
{
    DBusMessageIter args;
    DBusMessageIter results;

    if (!dbus_message_iter_init(message, &args) || dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_UINT32)
    {
        throw PlayerRuntimeError{"PortalScreenShoter", "Invalid portal response"};
    }

    uint32_t response = 1;
    dbus_message_iter_get_basic(&args, &response);
    if (response != 0)
    {
        std::ostringstream error;
        error << "Portal response=" << response << " (" << portalResponseMeaning(response)
              << "). Set " << InteractiveEnvVar << "=1 to test whether the desktop requires user interaction";
        throw PlayerRuntimeError{"PortalScreenShoter", error.str()};
    }

    if (!dbus_message_iter_next(&args) || dbus_message_iter_get_arg_type(&args) != DBUS_TYPE_ARRAY)
    {
        throw PlayerRuntimeError{"PortalScreenShoter", "Portal response did not include results"};
    }

    dbus_message_iter_recurse(&args, &results);
    auto summaryIter = results;
    Log::debug("[PortalScreenShoter] Portal response keys: [{}]", resultKeysSummary(summaryIter));

    while (dbus_message_iter_get_arg_type(&results) == DBUS_TYPE_DICT_ENTRY)
    {
        DBusMessageIter entry;
        DBusMessageIter variant;
        const char* key = nullptr;

        dbus_message_iter_recurse(&results, &entry);
        dbus_message_iter_get_basic(&entry, &key);
        dbus_message_iter_next(&entry);
        dbus_message_iter_recurse(&entry, &variant);

        if (key && std::string{key} == "uri" && dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_STRING)
        {
            const char* uri = nullptr;
            dbus_message_iter_get_basic(&variant, &uri);
            return uri ? uri : "";
        }

        dbus_message_iter_next(&results);
    }

    throw PlayerRuntimeError{"PortalScreenShoter", "Portal response did not include a screenshot URI"};
}

ImageBuffer PortalScreenShoter::readFileUri(const std::string& uri)
{
    auto path = filePathFromUri(uri);
    std::ifstream file{path, std::ios::binary};
    if (!file)
    {
        throw PlayerRuntimeError{"PortalScreenShoter", "Unable to open screenshot file"};
    }

    return ImageBuffer{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
}

std::string PortalScreenShoter::filePathFromUri(const std::string& uri)
{
    constexpr const char* FilePrefix = "file://";
    if (uri.rfind(FilePrefix, 0) != 0)
    {
        throw PlayerRuntimeError{"PortalScreenShoter", "Portal returned a non-file screenshot URI"};
    }

    std::string path;
    auto encoded = uri.substr(std::string{FilePrefix}.size());
    for (size_t i = 0; i < encoded.size(); ++i)
    {
        if (encoded[i] == '%' && i + 2 < encoded.size())
        {
            path.push_back(static_cast<char>(hexToByte(encoded[i + 1], encoded[i + 2])));
            i += 2;
        }
        else
        {
            path.push_back(encoded[i]);
        }
    }

    return path;
}
