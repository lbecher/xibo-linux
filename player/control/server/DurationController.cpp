#include "DurationController.hpp"

#include "common/logger/Logging.hpp"
#include "common/parsing/Parsing.hpp"

namespace
{
constexpr const char* ServerName = "Xibo Embedded Server";
}

DurationController::DurationController(DurationReceived callback) : callback_(std::move(callback)) {}

bool DurationController::matches(const HttpRequest& request) const
{
    if (request.method() != http::verb::post) return false;

    const auto target = std::string(request.target());
    return target == "/duration/expire" || target == "/duration/expire/" || target == "/duration/extend" ||
        target == "/duration/extend/" || target == "/duration/set" || target == "/duration/set/";
}

HttpResponse DurationController::handle(const HttpRequest& request) const
{
    if (!callback_)
    {
        HttpStringResponse response{http::status::internal_server_error, request.version()};
        response.set(http::field::server, ServerName);
        response.set(http::field::content_type, "text/html");
        response.keep_alive(request.keep_alive());
        response.body() = "Duration callback is not configured";
        response.prepare_payload();
        return response;
    }

    try
    {
        auto tree = Parsing::jsonFromString(request.body());

        DurationRequest duration;
        duration.id = tree.get("id", 0);
        duration.duration = tree.get("duration", 0);

        const auto target = std::string(request.target());
        if (target.find("/expire") != std::string::npos) duration.operation = "expire";
        if (target.find("/extend") != std::string::npos) duration.operation = "extend";
        if (target.find("/set") != std::string::npos) duration.operation = "set";

        callback_(duration);

        HttpStringResponse response{http::status::ok, request.version()};
        response.set(http::field::server, ServerName);
        response.set(http::field::content_type, "application/json");
        response.keep_alive(request.keep_alive());
        response.body() = "{\"success\":true}";
        response.prepare_payload();
        return response;
    }
    catch (const std::exception& e)
    {
        Log::error("[DurationController] Invalid duration payload: {}", e.what());

        HttpStringResponse response{http::status::bad_request, request.version()};
        response.set(http::field::server, ServerName);
        response.set(http::field::content_type, "text/html");
        response.keep_alive(request.keep_alive());
        response.body() = "Invalid duration payload";
        response.prepare_payload();
        return response;
    }
}
