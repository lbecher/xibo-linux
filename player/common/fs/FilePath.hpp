#pragma once

#include <boost/filesystem/path.hpp>
#include <fmt/format.h>

class FilePath : public boost::filesystem::path
{
public:
    using boost::filesystem::path::path;

    FilePath(const boost::filesystem::path& p) : boost::filesystem::path(p) {}
};

// Formatter specialization for fmt
template <>
struct fmt::formatter<FilePath> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const FilePath& path, FormatContext& ctx) {
        return format_to(ctx.out(), "{}", path.string());
    }
};
