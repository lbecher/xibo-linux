#include "UnsafeItemStore.hpp"

#include "common/parsing/Parsing.hpp"

#include <algorithm>

UnsafeItemStore& UnsafeItemStore::instance()
{
    static UnsafeItemStore store;
    return store;
}

void UnsafeItemStore::addUnsafeWidget(int code, const std::string& widgetId, const std::string& reason, int ttl)
{
    addUnsafeItem(UnsafeItemType::Widget, code, 0, widgetId, reason, ttl);
}

void UnsafeItemStore::addUnsafeItem(UnsafeItemType type,
                                    int code,
                                    int layoutId,
                                    const std::string& id,
                                    const std::string& reason,
                                    int ttl)
{
    cleanupExpired();

    if (ttl == 0)
    {
        ttl = DefaultUnsafeTtl;
    }

    for (auto& item : items_)
    {
        if (item.type == type && item.layoutId == layoutId && item.id == id)
        {
            item.code = code;
            item.reason = reason;
            item.ttl = ttl;
            item.dateTime = DateTime::now();
            return;
        }
    }

    items_.push_back(UnsafeItem{DateTime::now(), type, code, layoutId, id, reason, ttl});
}

void UnsafeItemStore::removeUnsafeLayout(int layoutId)
{
    cleanupExpired();

    items_.erase(std::remove_if(items_.begin(),
                                items_.end(),
                                [layoutId](const UnsafeItem& item) { return item.layoutId == layoutId; }),
                 items_.end());
}

bool UnsafeItemStore::isUnsafeLayout(int layoutId)
{
    cleanupExpired();

    for (auto&& item : items_)
    {
        if (item.layoutId == layoutId)
        {
            return true;
        }
    }

    return false;
}

bool UnsafeItemStore::isUnsafeMedia(const std::string& mediaId)
{
    cleanupExpired();

    for (auto&& item : items_)
    {
        if (item.type == UnsafeItemType::Media && item.id == mediaId)
        {
            return true;
        }
    }

    return false;
}

bool UnsafeItemStore::isUnsafeWidget(const std::string& widgetId)
{
    cleanupExpired();

    for (auto&& item : items_)
    {
        if (item.type == UnsafeItemType::Widget && item.id == widgetId)
        {
            return true;
        }
    }

    return false;
}

std::string UnsafeItemStore::listAsString()
{
    cleanupExpired();

    std::string result;
    for (auto&& item : items_)
    {
        result += item.id + ": " + item.reason + "\n";
    }
    return result;
}

std::string UnsafeItemStore::listAsJsonString()
{
    cleanupExpired();

    JsonNode root;
    for (auto&& item : items_)
    {
        JsonNode node;
        node.put("date", item.dateTime.string("%Y-%m-%d %H:%M:%S"));
        node.put("expires", (item.dateTime + DateTime::Seconds(item.ttl)).string("%Y-%m-%d %H:%M:%S"));
        node.put("type", toString(item.type));
        node.put("code", item.code);
        node.put("reason", item.reason);
        node.put("id", item.id);
        node.put("widgetId", item.type == UnsafeItemType::Widget ? item.id : "");
        node.put("layoutId", item.layoutId);
        root.push_back(std::make_pair("", node));
    }

    return Parsing::jsonToString(root);
}

void UnsafeItemStore::cleanupExpired()
{
    auto now = DateTime::now();
    items_.erase(std::remove_if(items_.begin(),
                                items_.end(),
                                [now](const UnsafeItem& item) { return item.dateTime + DateTime::Seconds(item.ttl) <= now; }),
                 items_.end());
}

std::string UnsafeItemStore::toString(UnsafeItemType type)
{
    switch (type)
    {
        case UnsafeItemType::Layout: return "layout";
        case UnsafeItemType::Region: return "region";
        case UnsafeItemType::Widget: return "widget";
        case UnsafeItemType::Media: return "media";
    }

    return "unknown";
}
