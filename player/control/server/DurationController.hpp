#pragma once

#include "control/server/DurationRequest.hpp"
#include "control/server/HttpTypes.hpp"

#include <functional>

using DurationReceived = std::function<void(const DurationRequest&)>;

class DurationController
{
public:
    explicit DurationController(DurationReceived callback);

    bool matches(const HttpRequest& request) const;
    HttpResponse handle(const HttpRequest& request) const;

private:
    DurationReceived callback_;
};
