#include "FaultController.hpp"

#include "common/logger/Logging.hpp"
#include "common/parsing/Parsing.hpp"

namespace
{
constexpr const char* ServerName = "Xibo Embedded Server";
}

FaultController::FaultController(FaultReceived callback) : callback_(std::move(callback)) {}

bool FaultController::matches(const HttpRequest& request) const
{
    return request.method() == http::verb::post && (request.target() == "/fault" || request.target() == "/fault/");
}

HttpResponse FaultController::handle(const HttpRequest& request) const
{
    if (!callback_)
    {
        HttpStringResponse response{http::status::internal_server_error, request.version()};
        response.set(http::field::server, ServerName);
        response.set(http::field::content_type, "text/html");
        response.keep_alive(request.keep_alive());
        response.body() = "Fault callback is not configured";
        response.prepare_payload();
        return response;
    }

    try
    {
        auto tree = Parsing::jsonFromString(request.body());

        FaultRequest fault;
        fault.code = tree.get("code", 0);
        fault.key = tree.get<std::string>("key");
        fault.ttl = tree.get("ttl", 0);
        fault.reason = tree.get<std::string>("reason", "");

        callback_(fault);

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
        Log::error("[FaultController] Invalid fault payload: {}", e.what());

        HttpStringResponse response{http::status::bad_request, request.version()};
        response.set(http::field::server, ServerName);
        response.set(http::field::content_type, "text/html");
        response.keep_alive(request.keep_alive());
        response.body() = "Invalid fault payload";
        response.prepare_payload();
        return response;
    }
}
