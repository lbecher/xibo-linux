#pragma once

#include "control/server/CriteriaRequest.hpp"
#include "control/server/HttpTypes.hpp"

#include <functional>

using CriteriaReceived = std::function<void(const CriteriaRequests&)>;

class CriteriaController
{
public:
    explicit CriteriaController(CriteriaReceived callback);

    bool matches(const HttpRequest& request) const;
    HttpResponse handle(const HttpRequest& request) const;

private:
    CriteriaReceived callback_;
};
