#pragma once

#include "common/dt/DateTime.hpp"

#include <string>
#include <vector>

constexpr int DefaultUnsafeTtl = 86400;

enum class UnsafeItemType
{
    Layout,
    Region,
    Widget,
    Media
};

struct UnsafeItem
{
    DateTime dateTime;
    UnsafeItemType type = UnsafeItemType::Widget;
    int code = 0;
    int layoutId = 0;
    std::string id;
    std::string reason;
    int ttl = 0;
};

class UnsafeItemStore
{
public:
    static UnsafeItemStore& instance();

    void addUnsafeWidget(int code, const std::string& widgetId, const std::string& reason, int ttl);
    void addUnsafeItem(UnsafeItemType type,
                       int code,
                       int layoutId,
                       const std::string& id,
                       const std::string& reason,
                       int ttl = DefaultUnsafeTtl);
    void removeUnsafeLayout(int layoutId);
    bool isUnsafeLayout(int layoutId);
    bool isUnsafeMedia(const std::string& mediaId);
    bool isUnsafeWidget(const std::string& widgetId);
    std::string listAsString();
    std::string listAsJsonString();

private:
    UnsafeItemStore() = default;

    void cleanupExpired();
    std::string toString(UnsafeItemType type);

private:
    std::vector<UnsafeItem> items_;
};
