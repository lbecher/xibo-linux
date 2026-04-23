#pragma once

#include "control/server/HttpTypes.hpp"

#include <functional>

using InfoResponseFactory = std::function<std::string()>;

class InfoController
{
public:
    explicit InfoController(InfoResponseFactory factory);

    bool matches(const HttpRequest& request) const;
    HttpResponse handle(const HttpRequest& request) const;

private:
    InfoResponseFactory factory_;
};
