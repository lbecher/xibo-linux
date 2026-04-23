#include "ScheduleItem.hpp"

#include "common/Utils.hpp"

bool operator==(const ScheduleCriteria& first, const ScheduleCriteria& second)
{
    if (first.metric != second.metric) return false;
    if (first.value != second.value) return false;
    if (first.type != second.type) return false;
    if (first.condition != second.condition) return false;

    return true;
}

bool operator!=(const ScheduleCriteria& first, const ScheduleCriteria& second)
{
    return !(first == second);
}

bool operator==(const DefaultScheduledLayout& first, const DefaultScheduledLayout& second)
{
    if (first.id != second.id) return false;
    if (!Utils::containersEqual(first.dependants, second.dependants)) return false;

    return true;
}

bool operator!=(const DefaultScheduledLayout& first, const DefaultScheduledLayout& second)
{
    return !(first == second);
}

bool operator==(const ScheduledLayout& first, const ScheduledLayout& second)
{
    if (first.scheduleId != second.scheduleId) return false;
    if (first.id != second.id) return false;
    if (first.priority != second.priority) return false;
    if (first.startDT != second.startDT || first.endDT != second.endDT) return false;
    if (!Utils::containersEqual(first.dependants, second.dependants)) return false;
    if (!Utils::containersEqual(first.criterias, second.criterias)) return false;

    return true;
}

bool operator!=(const ScheduledLayout& first, const ScheduledLayout& second)
{
    return !(first == second);
}
