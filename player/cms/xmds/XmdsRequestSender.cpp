#include "XmdsRequestSender.hpp"

#include "cms/xmds/Resources.hpp"
#include "cms/xmds/SoapRequestSender.hpp"

#include "common/crypto/RsaManager.hpp"
#include "common/system/System.hpp"
#include "config/AppConfig.hpp"
#include "xmr/XmrChannel.hpp"

#include <sys/utsname.h>

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

std::string operatingSystemJson()
{
    utsname info{};
    if (uname(&info) != 0)
    {
        return R"({"version":"Linux"})";
    }

    return std::string{R"({"version":")"} + jsonEscape(std::string{info.sysname} + " " + info.release) + R"("})";
}
}

const std::string DefaultClientType = "linux";
const std::string XmdsTarget = "/xmds.php?v=" + AppConfig::xmdsVersion();

XmdsRequestSender::XmdsRequestSender(const std::string& host,
                                     const std::string& serverKey,
                                     const std::string& hardwareKey) :
    uri_(Uri::fromString(host + XmdsTarget)),
    host_(host),
    serverKey_(serverKey),
    hardwareKey_(hardwareKey)
{
}

FutureResponseResult<RegisterDisplay::Result> XmdsRequestSender::registerDisplay(int clientCode,
                                                                                 const std::string& clientVersion,
                                                                                 const std::string& displayName)
{
    RegisterDisplay::Request request;
    request.serverKey = serverKey_;
    request.hardwareKey = hardwareKey_;
    request.clientType = DefaultClientType;
    request.clientCode = clientCode;
    request.clientVersion = clientVersion;
    request.macAddress = static_cast<std::string>(System::macAddress());
    request.operatingSystem = operatingSystemJson();
    request.xmrChannel = static_cast<std::string>(XmrChannel::fromCmsSettings(host_, serverKey_, hardwareKey_));
    request.xmrPubKey = CryptoUtils::keyToString(RsaManager::instance().publicKey());
    request.displayName = displayName;
    request.licenceResult = "";

    return SoapRequestHelper::sendRequest<RegisterDisplay::Result>(uri_, request);
}

FutureResponseResult<RequiredFiles::Result> XmdsRequestSender::requiredFiles()
{
    RequiredFiles::Request request;
    request.serverKey = serverKey_;
    request.hardwareKey = hardwareKey_;

    return SoapRequestHelper::sendRequest<RequiredFiles::Result>(uri_, request);
}

FutureResponseResult<Schedule::Result> XmdsRequestSender::schedule()
{
    Schedule::Request request;
    request.serverKey = serverKey_;
    request.hardwareKey = hardwareKey_;

    return SoapRequestHelper::sendRequest<Schedule::Result>(uri_, request);
}

FutureResponseResult<GetResource::Result> XmdsRequestSender::getResource(int layoutId, int regionId, int mediaId)
{
    GetResource::Request request;
    request.serverKey = serverKey_;
    request.hardwareKey = hardwareKey_;
    request.layoutId = layoutId;
    request.regionId = std::to_string(regionId);
    request.mediaId = std::to_string(mediaId);

    return SoapRequestHelper::sendRequest<GetResource::Result>(uri_, request);
}

FutureResponseResult<GetFile::Result> XmdsRequestSender::getFile(const std::string& fileId,
                                                                 const std::string& fileType,
                                                                 std::size_t chunkOffset,
                                                                 std::size_t chunkSize)
{
    GetFile::Request request;
    request.serverKey = serverKey_;
    request.hardwareKey = hardwareKey_;
    request.fileId = fileId;
    request.fileType = fileType;
    request.chunkOffset = chunkOffset;
    request.chunkSize = chunkSize;

    return SoapRequestHelper::sendRequest<GetFile::Result>(uri_, request);
}

FutureResponseResult<GetFile::Result> XmdsRequestSender::getDependency(const std::string& fileType,
                                                                       const std::string& id,
                                                                       std::size_t chunkOffset,
                                                                       std::size_t chunkSize)
{
    GetDependency::Request request;
    request.serverKey = serverKey_;
    request.hardwareKey = hardwareKey_;
    request.fileType = fileType;
    request.id = id;
    request.chunkOffset = chunkOffset;
    request.chunkSize = chunkSize;

    return SoapRequestHelper::sendRequest<GetFile::Result>(uri_, request);
}

FutureResponseResult<MediaInventory::Result> XmdsRequestSender::mediaInventory(MediaInventoryItems&& inventory)
{
    MediaInventory::Request request;
    request.serverKey = serverKey_;
    request.hardwareKey = hardwareKey_;
    request.inventory = std::move(inventory);

    return SoapRequestHelper::sendRequest<MediaInventory::Result>(uri_, request);
}

FutureResponseResult<SubmitLog::Result> XmdsRequestSender::submitLogs(const std::string& logXml)
{
    SubmitLog::Request request;
    request.serverKey = serverKey_;
    request.hardwareKey = hardwareKey_;
    request.logXml = std::string("<![CDATA[") + logXml + "]]>";

    return SoapRequestHelper::sendRequest<SubmitLog::Result>(uri_, request);
}

FutureResponseResult<SubmitStats::Result> XmdsRequestSender::submitStats(const std::string& statXml)
{
    SubmitStats::Request request;
    request.serverKey = serverKey_;
    request.hardwareKey = hardwareKey_;
    request.statXml = std::string("<![CDATA[") + statXml + "]]>";

    return SoapRequestHelper::sendRequest<SubmitStats::Result>(uri_, request);
}

FutureResponseResult<SubmitScreenShot::Result> XmdsRequestSender::submitScreenShot(const std::string& screenShot)
{
    SubmitScreenShot::Request request;
    request.serverKey = serverKey_;
    request.hardwareKey = hardwareKey_;
    request.screenShot = screenShot;

    return SoapRequestHelper::sendRequest<SubmitScreenShot::Result>(uri_, request);
}

FutureResponseResult<NotifyStatus::Result> XmdsRequestSender::notifyStatus(const std::string& status)
{
    NotifyStatus::Request request;
    request.serverKey = serverKey_;
    request.hardwareKey = hardwareKey_;
    request.status = status;

    return SoapRequestHelper::sendRequest<NotifyStatus::Result>(uri_, request);
}
