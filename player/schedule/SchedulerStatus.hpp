#pragma once

#include <string>
#include <vector>

struct SchedulerStatus
{
    std::vector<int> scheduledLayouts;
    std::vector<int> validLayouts;
    std::vector<int> invalidLayouts;
    std::vector<std::string> activeCriteria;
    int currentLayout;
    std::string generatedTime;
    bool weatherCriteriaActive = false;
};
