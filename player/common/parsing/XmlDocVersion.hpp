#pragma once

#include "common/types/internal/StrongType.hpp"
#include <string>
#include <fmt/format.h>

class XmlDocVersion : public StrongType<std::string>
{
public:
    using StrongType::StrongType;

    friend std::istream& operator>>(std::istream& in, XmlDocVersion& version);
    friend std::ostream& operator<<(std::ostream& out, const XmlDocVersion& version);
};

bool operator==(const XmlDocVersion& first, const XmlDocVersion& second);
bool operator!=(const XmlDocVersion& first, const XmlDocVersion& second);

std::istream& operator>>(std::istream& in, XmlDocVersion& version);
std::ostream& operator<<(std::ostream& out, const XmlDocVersion& version);

// Formatter specialization for fmt
template <>
struct fmt::formatter<XmlDocVersion> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const XmlDocVersion& version, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", static_cast<std::string>(version));
    }
};
