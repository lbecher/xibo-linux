#include "FileCacheImpl.hpp"

#include "common/fs/FileSystem.hpp"
#include "common/fs/Resource.hpp"
#include "common/logger/Logging.hpp"
#include "common/parsing/XmlFileLoaderMissingRoot.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <regex>

const char DefaultSeparator{'|'};
const NodePath ValidAttr{"valid", DefaultSeparator};
const NodePath Md5Attr{"md5", DefaultSeparator};
const NodePath LastUpdateAttr{"updated", DefaultSeparator};
const NodePath NameAttr{"<xmlattr>|name", DefaultSeparator};
const NodePath VersionAttr{"<xmlattr>|version", DefaultSeparator};
const NodePath RootNode{"cache", DefaultSeparator};
const NodePath FilesNode{"files", DefaultSeparator};
const std::string FileNodeName{"file"};
const std::string XmlAttrNodeName{"<xmlattr>"};

static std::string digitsOnly(const std::string& value)
{
    std::string result;
    result.reserve(value.size());

    for (auto ch : value)
    {
        if (std::isdigit(static_cast<unsigned char>(ch)))
        {
            result.push_back(ch);
        }
    }

    return result;
}

static boost::optional<int> parseTimestamp(const std::string& rawTimestamp)
{
    auto cleaned = digitsOnly(rawTimestamp);
    if (cleaned.empty())
    {
        return {};
    }

    try
    {
        return std::stoi(cleaned);
    }
    catch (const std::exception&)
    {
        return {};
    }
}

static bool parseBoolValue(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) { return std::tolower(ch); });
    return value == "1" || value == "true" || value == "yes";
}

static XmlNode& ensureFilesNode(XmlNode& cache)
{
    if (!cache.get_child_optional(RootNode))
    {
        cache.put_child(RootNode, XmlNode{});
    }

    if (!cache.get_child_optional(RootNode / FilesNode))
    {
        cache.put_child(RootNode / FilesNode, XmlNode{});
    }

    return cache.get_child(RootNode / FilesNode);
}

static const XmlNode* findFileNode(const XmlNode& cache, const std::string& filename)
{
    if (auto files = cache.get_child_optional(RootNode / FilesNode))
    {
        for (const auto& [entryName, node] : files.get())
        {
            if (entryName == FileNodeName && node.get<std::string>(NameAttr, "") == filename)
            {
                return &node;
            }
            if (entryName == filename)
            {
                return &node;
            }
        }
    }

    return nullptr;
}

static XmlNode* findFileNode(XmlNode& cache, const std::string& filename)
{
    if (auto files = cache.get_child_optional(RootNode / FilesNode))
    {
        for (auto& [entryName, node] : files.get())
        {
            if (entryName == FileNodeName && node.get<std::string>(NameAttr, "") == filename)
            {
                return &node;
            }
            if (entryName == filename)
            {
                return &node;
            }
        }
    }

    return nullptr;
}

static void upsertFileNode(XmlNode& cache, const std::string& filename, XmlNode&& newNode)
{
    auto& files = ensureFilesNode(cache);

    for (auto& [entryName, node] : files)
    {
        if ((entryName == FileNodeName && node.get<std::string>(NameAttr, "") == filename) || entryName == filename)
        {
            node = std::move(newNode);
            return;
        }
    }

    files.push_back(std::make_pair(FileNodeName, std::move(newNode)));
}

static bool loadLegacyInvalidCache(const FilePath& path, XmlNode& outCache)
{
    std::ifstream stream{path.string()};
    if (!stream.is_open())
    {
        return false;
    }

    std::string xml{std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
    if (xml.empty())
    {
        return false;
    }

    const std::regex entryRegex(
        R"(<([^/\s>]+)>\s*<md5>([^<]*)</md5>\s*(?:<updated>([^<]*)</updated>\s*)?<valid>([^<]*)</valid>\s*</\1>)");

    XmlNode migrated;
    auto& files = ensureFilesNode(migrated);
    bool hasRecoveredEntries = false;

    for (std::sregex_iterator it(xml.begin(), xml.end(), entryRegex), end; it != end; ++it)
    {
        auto filename = (*it)[1].str();
        auto md5 = (*it)[2].str();
        auto updated = (*it)[3].str();
        auto valid = (*it)[4].str();

        XmlNode entry;
        entry.put(NameAttr, filename);
        entry.put(Md5Attr, md5);

        auto parsedTimestamp = parseTimestamp(updated);
        if (parsedTimestamp)
        {
            entry.put(LastUpdateAttr, std::to_string(parsedTimestamp.value()));
        }
        entry.put(ValidAttr, parseBoolValue(valid));

        files.push_back(std::make_pair(FileNodeName, std::move(entry)));
        hasRecoveredEntries = true;
    }

    if (hasRecoveredEntries)
    {
        outCache = std::move(migrated);
    }

    return hasRecoveredEntries;
}

static bool normalizeCacheShape(XmlNode& cache)
{
    auto filesOpt = cache.get_child_optional(RootNode / FilesNode);
    if (!filesOpt)
    {
        return false;
    }

    XmlNode normalizedFiles;
    bool changed = false;

    for (const auto& [entryName, node] : filesOpt.get())
    {
        if (entryName == XmlAttrNodeName)
        {
            continue;
        }

        auto normalizedNode = node;
        auto fileName = entryName == FileNodeName ? node.get<std::string>(NameAttr, "") : entryName;
        if (fileName.empty())
        {
            continue;
        }

        if (normalizedNode.get<std::string>(NameAttr, "") != fileName)
        {
            normalizedNode.put(NameAttr, fileName);
            changed = true;
        }

        if (auto updatedRaw = normalizedNode.get_optional<std::string>(LastUpdateAttr))
        {
            auto parsedTimestamp = parseTimestamp(updatedRaw.value());
            if (parsedTimestamp)
            {
                auto canonicalTimestamp = std::to_string(parsedTimestamp.value());
                if (updatedRaw.value() != canonicalTimestamp)
                {
                    normalizedNode.put(LastUpdateAttr, canonicalTimestamp);
                    changed = true;
                }
            }
        }

        normalizedFiles.push_back(std::make_pair(FileNodeName, std::move(normalizedNode)));
        if (entryName != FileNodeName)
        {
            changed = true;
        }
    }

    if (changed)
    {
        cache.put_child(RootNode / FilesNode, std::move(normalizedFiles));
    }

    return changed;
}

void FileCacheImpl::loadFrom(const FilePath& cacheFile)
{
    cacheFile_ = cacheFile;
    loadFileHashes(cacheFile_);
}

bool FileCacheImpl::valid(const std::string& filename) const
{
    boost::mutex::scoped_lock lock(fileCacheMutex_);
    auto node = findFileNode(fileCache_, filename);

    return node && node->get(ValidAttr, false);
}

bool FileCacheImpl::cached(const RegularFile& file) const
{
    return cached(file.name(), file.hash());
}

bool FileCacheImpl::cached(const ResourceFile& file) const
{
    boost::mutex::scoped_lock lock(fileCacheMutex_);
    auto node = findFileNode(fileCache_, file.name());

    if (node)
    {
        auto lastUpdate = node->get_optional<std::string>(LastUpdateAttr);
        if (lastUpdate.has_value())
        {
            auto parsedTimestamp = parseTimestamp(lastUpdate.value());
            if (!parsedTimestamp)
            {
                return false;
            }

            auto savedLastUpdated = DateTime::utcFromTimestamp(parsedTimestamp.value());
            return savedLastUpdated >= file.lastUpdate();
        }
    }

    return false;
}

bool FileCacheImpl::cached(const std::string& filename, const Md5Hash& hash) const
{
    boost::mutex::scoped_lock lock(fileCacheMutex_);
    auto node = findFileNode(fileCache_, filename);

    if (node && node->get_optional<std::string>(Md5Attr))
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
    boost::mutex::scoped_lock lock(fileCacheMutex_);
    auto node = findFileNode(fileCache_, filename);
    if (!node)
    {
        return false;
    }

    return node->get_optional<std::string>(LastUpdateAttr).has_value();
}

std::vector<std::string> FileCacheImpl::cachedFiles() const
{
    boost::mutex::scoped_lock lock(fileCacheMutex_);
    std::vector<std::string> files;
    if (auto root = fileCache_.get_child_optional(RootNode / FilesNode))
    {
        for (auto&& [name, node] : root.value())
        {
            if (name == FileNodeName)
            {
                auto fileName = node.get<std::string>(NameAttr, "");
                if (!fileName.empty())
                {
                    files.push_back(fileName);
                }
                continue;
            }

            if (name != XmlAttrNodeName)
            {
                files.push_back(name);
            }
        }
    }
    return files;
}

std::vector<std::string> FileCacheImpl::invalidFiles() const
{
    boost::mutex::scoped_lock lock(fileCacheMutex_);
    std::vector<std::string> files;
    if (auto root = fileCache_.get_child_optional(RootNode / FilesNode))
    {
        for (auto&& [name, node] : root.value())
        {
            if (name == FileNodeName)
            {
                auto fileName = node.get<std::string>(NameAttr, "");
                if (!fileName.empty() && !node.get(ValidAttr, false))
                {
                    files.push_back(fileName);
                }
                continue;
            }

            if (name != XmlAttrNodeName && !node.get(ValidAttr, false))
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
    boost::mutex::scoped_lock lock(fileCacheMutex_);
    auto node = findFileNode(fileCache_, filename);

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
    node.put(NameAttr, filename);
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

    boost::mutex::scoped_lock lock(fileCacheMutex_);
    upsertFileNode(fileCache_, filename, std::move(node));
    saveFileHashes(cacheFile_);
}

void FileCacheImpl::addToCache(const std::string& filename, const Md5Hash& hash, const DateTime& lastUpdate)
{
    XmlNode node;
    node.put(NameAttr, filename);
    node.put(Md5Attr, hash);
    node.put(LastUpdateAttr, std::to_string(lastUpdate.timestamp()));
    node.put(ValidAttr, true);

    boost::mutex::scoped_lock lock(fileCacheMutex_);
    upsertFileNode(fileCache_, filename, std::move(node));
    saveFileHashes(cacheFile_);
}

void FileCacheImpl::loadFileHashes(const FilePath& path)
{
    try
    {
        fileCache_ = loadXmlFrom(path);

        bool normalized = false;
        {
            boost::mutex::scoped_lock lock(fileCacheMutex_);
            normalized = normalizeCacheShape(fileCache_);
        }

        if (normalized)
        {
            saveFileHashes(path);
            Log::info("[FileCache] Normalized cache format at {}", path.string());
        }
    }
    catch (PlayerRuntimeError& e)
    {
        if (FileSystem::exists(path))
        {
            XmlNode migratedCache;
            if (loadLegacyInvalidCache(path, migratedCache))
            {
                {
                    boost::mutex::scoped_lock lock(fileCacheMutex_);
                    fileCache_ = std::move(migratedCache);
                }
                saveFileHashes(path);
                Log::info("[FileCache] Migrated legacy cache format at {}", path.string());
                return;
            }
        }

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
