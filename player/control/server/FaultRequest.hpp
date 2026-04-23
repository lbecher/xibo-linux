#pragma once

#include <string>

struct FaultRequest
{
    int code = 0;
    std::string key;
    int ttl = 0;
    std::string reason;
};
