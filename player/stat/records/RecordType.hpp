#pragma once

#include <optional>
#include <string>

namespace Stats
{
    enum class RecordType
    {
        Layout,
        Media
    };

    std::string recordTypeToString(RecordType t);
    std::optional<RecordType> recordTypeFromSting(const std::string& t);
}
