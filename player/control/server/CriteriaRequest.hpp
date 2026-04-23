#pragma once

#include <string>
#include <vector>

struct CriteriaRequest
{
    std::string metric;
    std::string value;
    int ttl = 300;
};

using CriteriaRequests = std::vector<CriteriaRequest>;
