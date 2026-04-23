#include "RestrictiveFileModule.hpp"

#include "common/logger/Logging.hpp"

namespace
{
constexpr const char* ServerName = "Xibo Embedded Server";

beast::string_view mimeType(const FilePath& path)
{
    auto const extension = [path = path.string()] {
        auto const pos = path.rfind(".");
        if (pos == beast::string_view::npos) return std::string{};
        return path.substr(pos);
    }();

    if (extension == ".htm") return "text/html";
    if (extension == ".html") return "text/html";
    if (extension == ".php") return "text/html";
    if (extension == ".css") return "text/css";
    if (extension == ".txt") return "text/plain";
    if (extension == ".js") return "application/javascript";
    if (extension == ".json") return "application/json";
    if (extension == ".xml") return "application/xml";

    return "application/text";
}
}

RestrictiveFileModule::RestrictiveFileModule(FilePath rootDirectory) : rootDirectory_(std::move(rootDirectory)) {}

bool RestrictiveFileModule::matches(const HttpRequest& request) const
{
    return request.method() == http::verb::get;
}

HttpResponse RestrictiveFileModule::handle(const HttpRequest& request) const
{
    FilePath path{rootDirectory_.string() + std::string{request.target()}};

    beast::error_code ec;
    http::file_body::value_type body;
    body.open(path.c_str(), beast::file_mode::scan, ec);

    if (ec == beast::errc::no_such_file_or_directory)
    {
        Log::error("[RestrictiveFileModule] Not found: {}", path.string());

        HttpStringResponse response{http::status::not_found, request.version()};
        response.set(http::field::server, ServerName);
        response.set(http::field::content_type, "text/html");
        response.keep_alive(request.keep_alive());
        response.body() = "The resource '" + std::string(request.target()) + "' was not found.";
        response.prepare_payload();
        return response;
    }

    if (ec)
    {
        Log::error("[RestrictiveFileModule] File open error for {}: {}", path.string(), ec.message());

        HttpStringResponse response{http::status::internal_server_error, request.version()};
        response.set(http::field::server, ServerName);
        response.set(http::field::content_type, "text/html");
        response.keep_alive(request.keep_alive());
        response.body() = "An error occurred: '" + ec.message() + "'";
        response.prepare_payload();
        return response;
    }

    const auto size = body.size();

    HttpFileResponse response{
        std::piecewise_construct, std::make_tuple(std::move(body)), std::make_tuple(http::status::ok, request.version())};
    response.set(http::field::server, ServerName);
    response.set(http::field::content_type, mimeType(path));
    response.content_length(size);
    response.keep_alive(request.keep_alive());
    return response;
}
