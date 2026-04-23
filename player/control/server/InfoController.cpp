#include "InfoController.hpp"

namespace
{
constexpr const char* ServerName = "Xibo Embedded Server";
}

InfoController::InfoController(InfoResponseFactory factory) : factory_(std::move(factory)) {}

bool InfoController::matches(const HttpRequest& request) const
{
    return request.method() == http::verb::get && (request.target() == "/info" || request.target() == "/info/");
}

HttpResponse InfoController::handle(const HttpRequest& request) const
{
    HttpStringResponse response{http::status::ok, request.version()};
    response.set(http::field::server, ServerName);
    response.set(http::field::content_type, "application/json");
    response.keep_alive(request.keep_alive());
    response.body() = factory_ ? factory_() : "{}";
    response.prepare_payload();
    return response;
}
