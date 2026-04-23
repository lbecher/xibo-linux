#pragma once

#include <string>

struct DurationRequest
{
    std::string operation;
    int id = 0;
    int duration = 0;
};
