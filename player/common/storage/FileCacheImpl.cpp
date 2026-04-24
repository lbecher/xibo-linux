#include "FileCacheImpl.hpp"

#include "common/fs/FileSystem.hpp"
#include "common/fs/Resource.hpp"
#include "common/logger/Logging.hpp"
#include "common/parsing/XmlFileLoaderMissingRoot.hpp"

const char DefaultSeparator{'|'};
const NodePath ValidAttr{"valid", DefaultSeparator};
const NodePath Md5Attr{"md5", DefaultSeparator};
const NodePath LastUpdateAttr{"updated", DefaultSeparator};
const NodePath VersionAttr{"<xmlattr>|version", DefaultSeparator};
const NodePath RootNode{"cache", DefaultSeparator};
const NodePath FilesNode{"files", DefaultSeparator};

static NodePath fullPath(const std::string& path)
{
    return RootNode / FilesNode / NodePath{path, DefaultSeparator};
}

void FileCacheImpl::loadFrom(const FilePath& cacheFile)
{
    cacheFile_ = cacheFile;
    loadFileHashes(cacheFile_);
}

bool FileCacheImpl::valid(const std::string& filename) const
{
    auto node = fileCache_.get_child_optional(fullPath(filename));

    return node.has_value() && node->get<bool>(ValidAttr);
}

bool FileCacheImpl::cached(const RegularFile& file) const
{
    return cached(file.name(), file.hash());
}

bool FileCacheImpl::cached(const ResourceFile& file) const
{
    auto node = fileCache_.get_child_optional(fullPath(file.name()));

    if (node)
    {
        auto lastUpdate = node->get_optional<int>(LastUpdateAttr);
        if (lastUpdate)
        {
            auto savedLastUpdated = DateTime::utcFromTimestamp(*lastUpdate);
            return savedLastUpdated >= file.lastUpdate();
        }
    }

    return false;
}

bool FileCacheImpl::cached(const std::string& filename, const Md5Hash& hash) const
{
    auto node = fileCache_.get_child_optional(fullPath(filename));

    if (node)
    {
        Md5Hash savedHash{node->get<std::string>(Md5Attr)};
        if (savedHash != hash)
        {
            Log::error("[FileCache] Cache hash mismatch for {}: cached='{}' checked='{}'",
                       filename,
                       static_cast<std::string>(savedHash),
                       static_cast<std::string>(hash));
        }
        return savedHash == hash;
    }

    return false;
}

bool FileCacheImpl::usesTimestampValidation(const std::string& filename) const
{
    auto node = fileCache_.get_child_optional(fullPath(filename));
    if (!node)
    {
        return false;
    }

    return node->get_optional<int>(LastUpdateAttr).has_value();
}

std::vector<std::string> FileCacheImpl::cachedFiles() const
{
    std::vector<std::string> files;
    if (auto root = fileCache_.get_child_optional(RootNode / FilesNode))
    {
        for (auto&& [name, node] : root.value())
        {
            files.push_back(name);
        }
    }
    return files;
}

std::vector<std::string> FileCacheImpl::invalidFiles() const
{
    std::vector<std::string> files;
    if (auto root = fileCache_.get_child_optional(RootNode / FilesNode))
    {
        for (auto&& [name, node] : root.value())
        {
            if (!valid(name))
            {
                files.push_back(name);
            }
        }
    }
    return files;
}

void FileCacheImpl::save(const std::string& fileName, const std::string& fileContent, const Md5Hash& hash)
{
    Resource path{fileName};

    FileSystem::writeToFile(path, fileContent);

    auto savedHash = Md5Hash::fromFile(path);
    addToCache(fileName, savedHash, hash);
}

void FileCacheImpl::save(const std::string& fileName, const std::string& fileContent, const DateTime& lastUpdate)
{
    Resource path{fileName};

    FileSystem::writeToFile(path, fileContent);

    auto savedHash = Md5Hash::fromFile(path);
    addToCache(fileName, savedHash, lastUpdate);
}

void FileCacheImpl::markAsInvalid(const std::string& filename)
{
    auto node = fileCache_.get_child_optional(fullPath(filename));

    if (node)
    {
        node->put(ValidAttr, false);
    }
}

XmlDocVersion FileCacheImpl::currentVersion() const
{
    return XmlDocVersion{"2"};
}

NodePath FileCacheImpl::versionAttributePath() const
{
    return RootNode / VersionAttr;
}

std::unique_ptr<XmlFileLoader> FileCacheImpl::backwardCompatibleLoader(const XmlDocVersion& version) const
{
    if (version == XmlDocVersion{"1"}) return std::make_unique<XmlFileLoaderMissingRoot>(RootNode / FilesNode);
    return nullptr;
}

void FileCacheImpl::addToCache(const std::string& filename, const Md5Hash& hash, const Md5Hash& target)
{
    XmlNode node;
    node.put(Md5Attr, hash);
    node.put(ValidAttr, hash == target);

    if (hash != target)
    {
        Log::error("[FileCache] HASH MISMATCH for {}: calculated='{}' expected='{}' MARKING INVALID", 
                   filename, static_cast<std::string>(hash), static_cast<std::string>(target));
    }
    else
    {
        Log::info("[FileCache] Hash OK for {}: '{}'", filename, static_cast<std::string>(hash));
    }

    /* critical section, access and modify file cache buffer */
    fileCacheMutex_.lock();
    fileCache_.put_child(fullPath(filename), node);

    saveFileHashes(cacheFile_);
    fileCacheMutex_.unlock();
}

void FileCacheImpl::addToCache(const std::string& filename, const Md5Hash& hash, const DateTime& lastUpdate)
{
    XmlNode node;
    node.put(Md5Attr, hash);
    node.put(LastUpdateAttr, lastUpdate.timestamp());
    node.put(ValidAttr, true);

    /* critical section, access and modify file cache buffer */
    fileCacheMutex_.lock();
    fileCache_.put_child(fullPath(filename), node);

    saveFileHashes(cacheFile_);
    fileCacheMutex_.unlock();
}

void FileCacheImpl::loadFileHashes(const FilePath& path)
{
    try
    {
        fileCache_ = loadXmlFrom(path);
    }
    catch (PlayerRuntimeError& e)
    {
        Log::error("[FileCache] Load error: {}", e.message());
    }
    catch (std::exception& e)
    {
        Log::error("[FileCache] Load error: {}", e.what());
    }
}

void FileCacheImpl::saveFileHashes(const FilePath& path)
{
    try
    {
        saveXmlTo(path, fileCache_);
    }
    catch (PlayerRuntimeError& e)
    {
        Log::error("[FileCache] Save error: {}", e.message());
    }
    catch (std::exception& e)
    {
        Log::error("[FileCache] Save error: {}", e.what());
    }
}
