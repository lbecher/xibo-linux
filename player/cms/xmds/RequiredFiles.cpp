#include "RequiredFiles.hpp"

#include "cms/xmds/Resources.hpp"
#include "common/logger/Logging.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace Resources = XmdsResources::RequiredFiles;

const RequiredFilesSet<RegularFile>& RequiredFiles::Result::requiredFiles() const
{
    return m_requiredFiles;
}

const RequiredFilesSet<ResourceFile>& RequiredFiles::Result::requiredResources() const
{
    return m_requiredResources;
}

const RequiredFilesSet<WidgetDataFile>& RequiredFiles::Result::requiredWidgetData() const
{
    return m_requiredWidgetData;
}

void RequiredFiles::Result::addFile(RegularFile&& file)
{
    m_requiredFiles.emplace_back(std::move(file));
}

void RequiredFiles::Result::addResource(ResourceFile&& resource)
{
    m_requiredResources.emplace_back(std::move(resource));
}

void RequiredFiles::Result::addWidgetData(WidgetDataFile&& widgetData)
{
    m_requiredWidgetData.emplace_back(std::move(widgetData));
}

Soap::RequestSerializer<RequiredFiles::Request>::RequestSerializer(const RequiredFiles::Request& request) :
    BaseRequestSerializer(request)
{
}

std::string Soap::RequestSerializer<RequiredFiles::Request>::string()
{
    return createRequest(Resources::Name, request().serverKey, request().hardwareKey);
}

Soap::ResponseParser<RequiredFiles::Result>::ResponseParser(const std::string& soapResponse) :
    BaseResponseParser(soapResponse)
{
}

RequiredFiles::Result Soap::ResponseParser<RequiredFiles::Result>::parseBody(const XmlNode& node)
{
    auto requiredFilesXml = node.get<std::string>(Resources::RequiredFilesXml);
    auto filesNode = Parsing::xmlFrom(requiredFilesXml).get_child(Resources::Files);

    RequiredFiles::Result result;

    for (auto [name, fileNode] : filesNode)
    {
        if (name != Resources::File) continue;

        auto fileAttrs = fileNode.get_child(Resources::FileAttrs);
        auto fileType = fileAttrs.get<std::string>(Resources::FileType);

        if (isLayout(fileType) || isMedia(fileType) || isDependency(fileType))
        {
            result.addFile(parseRegularFile(fileAttrs));
        }
        else if (isResource(fileType))
        {
            result.addResource(parseResourceFile(fileAttrs));
        }
        else if (isWidgetData(fileType))
        {
            result.addWidgetData(parseWidgetDataFile(fileAttrs));
        }
    }

    return result;
}

RegularFile Soap::ResponseParser<RequiredFiles::Result>::parseRegularFile(const XmlNode& attrs)
{
    auto fileType = attrs.get<std::string>(Resources::FileType);
    auto remoteId = attrs.get<std::string>(Resources::RegularFile::Id);
    auto id = 0;
    try
    {
        id = std::stoi(remoteId);
    }
    catch (const std::exception&)
    {
        if (!isDependency(fileType))
        {
            Log::error("[RequiredFiles] Invalid numeric id read from XML - type: {}, id: '{}'", fileType, remoteId);
        }
    }
    auto size = attrs.get<size_t>(Resources::RegularFile::Size);
    auto md5Value = attrs.get<std::string>(Resources::RegularFile::MD5);
    auto md5 = Md5Hash{md5Value};
    auto downloadType = toDownloadType(attrs.get<std::string>(Resources::RegularFile::DownloadType));
    auto [path, name] = parseFileNameAndPath(downloadType, fileType, attrs);
    auto requestType = fileType;
    if (isDependency(fileType))
    {
        requestType = attrs.get<std::string>(Resources::RegularFile::DependencyFileType, requestType);
    }

    auto invalidMd5 = md5Value.size() != 32
        || std::any_of(md5Value.begin(), md5Value.end(), [](unsigned char ch) { return !std::isxdigit(ch); });
    if (invalidMd5)
    {
        Log::error("[RequiredFiles] Invalid hash format read from XML - id: {}, name: {}, md5: '{}'", id, name, md5Value);
    }

    Log::info("[RequiredFiles] Parsed regular file - id: {}, remoteId: {}, type: {}, requestType: {}, name: {}, md5: {}",
              id,
              remoteId,
              fileType,
              requestType,
              name,
              static_cast<std::string>(md5));

    return RegularFile{id, size, md5, path, name, fileType, downloadType, remoteId, requestType};
}

ResourceFile Soap::ResponseParser<RequiredFiles::Result>::parseResourceFile(const XmlNode& attrs)
{
    auto layoutId = attrs.get<int>(Resources::ResourceFile::LayoutId);
    auto regionId = attrs.get<int>(Resources::ResourceFile::RegionId);
    auto mediaId = attrs.get<int>(Resources::ResourceFile::MediaId);
    auto lastUpdate = DateTime::utcFromTimestamp(attrs.get<int>(Resources::ResourceFile::LastUpdate));

    return ResourceFile{layoutId, regionId, mediaId, lastUpdate};
}

WidgetDataFile Soap::ResponseParser<RequiredFiles::Result>::parseWidgetDataFile(const XmlNode& attrs)
{
    auto widgetId = attrs.get<int>(Resources::WidgetDataFile::Id);
    auto updateInterval = attrs.get<int>(Resources::WidgetDataFile::UpdateInterval, 0);

    return WidgetDataFile{widgetId, updateInterval};
}

bool Soap::ResponseParser<RequiredFiles::Result>::isLayout(std::string_view type) const
{
    return type == Resources::LayoutType;
}

bool Soap::ResponseParser<RequiredFiles::Result>::isMedia(std::string_view type) const
{
    return type == Resources::MediaType;
}

bool Soap::ResponseParser<RequiredFiles::Result>::isDependency(std::string_view type) const
{
    return type == Resources::DependencyType;
}

bool Soap::ResponseParser<RequiredFiles::Result>::isResource(std::string_view type) const
{
    return type == Resources::ResourceType;
}

bool Soap::ResponseParser<RequiredFiles::Result>::isWidgetData(std::string_view type) const
{
    return type == Resources::WidgetType;
}

RegularFile::DownloadType Soap::ResponseParser<RequiredFiles::Result>::toDownloadType(std::string_view type)
{
    if (type == Resources::RegularFile::HttpDownload)
        return RegularFile::DownloadType::HTTP;
    else if (type == Resources::RegularFile::XmdsDownload)
        return RegularFile::DownloadType::XMDS;

    return RegularFile::DownloadType::Invalid;
}

// NOTE: workaround because filePath and fileName from RequiredFiles request are a bit clumsy to parse directly
std::pair<std::string, std::string> Soap::ResponseParser<RequiredFiles::Result>::parseFileNameAndPath(
    RegularFile::DownloadType dType,
    std::string_view fType,
    const XmlNode& attrs)
{
    std::string path, name;

    switch (dType)
    {
        case RegularFile::DownloadType::HTTP:
            path = attrs.get<std::string>(Resources::RegularFile::Path);
            name = attrs.get<std::string>(Resources::RegularFile::Name);
            break;
        case RegularFile::DownloadType::XMDS:
            name = attrs.get<std::string>(Resources::RegularFile::Path);
            if (isLayout(fType))
            {
                name += ".xlf";
            }
            break;
        default: break;
    }

    return std::pair{path, name};
}
