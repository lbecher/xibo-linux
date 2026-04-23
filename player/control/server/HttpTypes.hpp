#pragma once

#include <boost/beast/http.hpp>
#include <variant>

namespace beast = boost::beast;
namespace http = beast::http;

using HttpRequest = http::request<http::string_body>;
using HttpStringResponse = http::response<http::string_body>;
using HttpFileResponse = http::response<http::file_body>;
using HttpResponse = std::variant<HttpStringResponse, HttpFileResponse>;
