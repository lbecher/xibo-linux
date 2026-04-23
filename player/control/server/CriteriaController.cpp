#include "CriteriaController.hpp"

#include "common/logger/Logging.hpp"
#include "common/parsing/Parsing.hpp"

namespace
{
constexpr const char* ServerName = "Xibo Embedded Server";
}

CriteriaController::CriteriaController(CriteriaReceived callback) : callback_(std::move(callback)) {}

bool CriteriaController::matches(const HttpRequest& request) const
{
    return request.method() == http::verb::post &&
        (request.target() == "/criteria" || request.target() == "/criteria/");
}

HttpResponse CriteriaController::handle(const HttpRequest& request) const
{
    if (!callback_)
    {
        HttpStringResponse response{http::status::internal_server_error, request.version()};
        response.set(http::field::server, ServerName);
        response.set(http::field::content_type, "text/html");
        response.keep_alive(request.keep_alive());
        response.body() = "Criteria callback is not configured";
        response.prepare_payload();
        return response;
    }

    try
    {
        CriteriaRequests updates;
        auto tree = Parsing::jsonFromString(request.body());
        for (auto&& [_, item] : tree)
        {
            CriteriaRequest criteria;
            criteria.metric = item.get<std::string>("metric");
            criteria.value = item.get<std::string>("value");
            criteria.ttl = item.get("ttl", 300);
            updates.emplace_back(std::move(criteria));
        }

        callback_(updates);

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
        Log::error("[CriteriaController] Invalid criteria payload: {}", e.what());

        HttpStringResponse response{http::status::bad_request, request.version()};
        response.set(http::field::server, ServerName);
        response.set(http::field::content_type, "text/html");
        response.keep_alive(request.keep_alive());
        response.body() = "Invalid criteria payload";
        response.prepare_payload();
        return response;
    }
}
