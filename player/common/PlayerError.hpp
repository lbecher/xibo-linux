#pragma once

#include <string>
#include <fmt/format.h>

class PlayerError
{
public:
    PlayerError() = default;
    PlayerError(std::string_view domain, std::string_view message);

    explicit operator bool() const noexcept;
    const std::string& domain() const;
    const std::string& message() const;

    friend std::ostream& operator<<(std::ostream& out, const PlayerError& error);

private:
    std::string domain_;
    std::string message_;
};

// Formatter specialization for fmt
template <>
struct fmt::formatter<PlayerError> {
    constexpr auto parse(format_parse_context& ctx) {
        return ctx.begin();
    }

    template <typename FormatContext>
    auto format(const PlayerError& error, FormatContext& ctx) {
        return format_to(ctx.out(), "[{}] {}", error.domain(), error.message());
    }
};
