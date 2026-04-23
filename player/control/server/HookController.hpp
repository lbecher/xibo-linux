#pragma once

#include "control/server/HttpTypes.hpp"
#include "control/server/TriggerRequest.hpp"

#include <functional>

using TriggerReceived = std::function<void(const TriggerRequest&)>;

class HookController
{
public:
    explicit HookController(TriggerReceived callback);

    bool matches(const HttpRequest& request) const;
    HttpResponse handle(const HttpRequest& request) const;

private:
    TriggerReceived callback_;
};
