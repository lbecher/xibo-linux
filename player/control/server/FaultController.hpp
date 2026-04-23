#pragma once

#include "control/server/FaultRequest.hpp"
#include "control/server/HttpTypes.hpp"

#include <functional>

using FaultReceived = std::function<void(const FaultRequest&)>;

class FaultController
{
public:
    explicit FaultController(FaultReceived callback);

    bool matches(const HttpRequest& request) const;
    HttpResponse handle(const HttpRequest& request) const;

private:
    FaultReceived callback_;
};
