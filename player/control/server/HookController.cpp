#include "HookController.hpp"

#include "common/logger/Logging.hpp"
#include "common/parsing/Parsing.hpp"

namespace
{
constexpr const char* ServerName = "Xibo Embedded Server";
}

HookController::HookController(TriggerReceived callback) : callback_(std::move(callback)) {}

bool HookController::matches(const HttpRequest& request) const
{
    return request.method() == http::verb::post && (request.target() == "/trigger" || request.target() == "/trigger/");
}

HttpResponse HookController::handle(const HttpRequest& request) const
{
    if (!callback_)
    {
        HttpStringResponse response{http::status::internal_server_error, request.version()};
        response.set(http::field::server, ServerName);
        response.set(http::field::content_type, "text/html");
        response.keep_alive(request.keep_alive());
        response.body() = "Trigger callback is not configured";
        response.prepare_payload();
        return response;
    }

    try
    {
        auto tree = Parsing::jsonFromString(request.body());

        TriggerRequest trigger;
        trigger.trigger = tree.get<std::string>("trigger");
        trigger.id = tree.get("id", 0);

        callback_(trigger);

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
        Log::error("[HookController] Invalid trigger payload: {}", e.what());

        HttpStringResponse response{http::status::bad_request, request.version()};
        response.set(http::field::server, ServerName);
        response.set(http::field::content_type, "text/html");
        response.keep_alive(request.keep_alive());
        response.body() = "Invalid trigger payload";
        response.prepare_payload();
        return response;
    }
}
