#pragma once

#include "common/storage/RequiredItems.hpp"

class MediaInventoryItem
{
public:
    MediaInventoryItem(const RegularFile& file, bool downloadComplete);
    MediaInventoryItem(const ResourceFile& file, bool downloadComplete);

    const std::string& type() const;
    const std::string& id() const;
    const std::string& fileType() const;
    bool downloadComplete() const;
    const Md5Hash& md5() const;
    const std::string& lastChecked() const;

private:
    MediaInventoryItem(bool downloadComplete);

    std::string type_;
    std::string id_;
    std::string fileType_;
    bool downloadComplete_;
    Md5Hash md5_;
    std::string lastChecked_;
};

using MediaInventoryItems = std::vector<MediaInventoryItem>;
