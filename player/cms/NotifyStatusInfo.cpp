#include "NotifyStatusInfo.hpp"

#include <sstream>

namespace
{
std::string jsonEscape(const std::string& value)
{
    std::string escaped;
    escaped.reserve(value.size());

    for (char ch : value)
    {
        switch (ch)
        {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default: escaped += ch; break;
        }
    }

    return escaped;
}

void appendSeparator(std::stringstream& stream, bool& first)
{
    if (!first)
    {
        stream << ",";
    }

    first = false;
}

void appendNumber(std::stringstream& stream, bool& first, const std::string& name, std::uintmax_t value)
{
    appendSeparator(stream, first);
    stream << "\"" << name << "\":" << std::to_string(value);
}

void appendNumber(std::stringstream& stream, bool& first, const std::string& name, LayoutId value)
{
    appendSeparator(stream, first);
    stream << "\"" << name << "\":" << std::to_string(value);
}

void appendString(std::stringstream& stream, bool& first, const std::string& name, const std::string& value)
{
    appendSeparator(stream, first);
    stream << "\"" << name << "\":\"" << jsonEscape(value) << "\"";
}
}

std::string NotifyStatusInfo::string() const
{
    std::stringstream stream;
    bool first = true;

    stream << "{";

    if (currentLayoutId > EmptyLayoutId)
    {
        appendNumber(stream, first, "currentLayoutId", currentLayoutId);
    }

    if (hasSpaceUsageInfo)
    {
        appendNumber(stream, first, "availableSpace", spaceUsageInfo.available);
        appendNumber(stream, first, "totalSpace", spaceUsageInfo.total);
    }

    //    tree.put("lastCommandSuccess", ""); TODO: implement when commands will be available
    if (!static_cast<std::string>(deviceName).empty())
    {
        appendString(stream, first, "deviceName", static_cast<std::string>(deviceName));
    }

    if (!timezone.empty())
    {
        appendString(stream, first, "timeZone", timezone);
    }

    stream << "}";

    return stream.str();
}
