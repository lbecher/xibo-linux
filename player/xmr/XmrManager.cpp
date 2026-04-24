#include "XmrManager.hpp"

#include "MainLoop.hpp"

#include "common/parsing/Parsing.hpp"
#include "common/crypto/RsaManager.hpp"
#include "common/dt/DateTime.hpp"
#include "common/logger/Logging.hpp"
#include "config/AppConfig.hpp"

#include <boost/algorithm/string/predicate.hpp>

const size_t CHANNEL_PART = 0;
const size_t KEY_PART = 1;
const size_t MESSAGE_PART = 2;

const char* const HearbeatChannel = "H";

std::string normalizeZmqEndpoint(const std::string& host)
{
    if (host.find("://") != std::string::npos)
    {
        return host;
    }

    return "tcp://" + host;
}

XmrManager::XmrManager(const XmrChannel& mainChannel) : mainChannel_(static_cast<std::string>(mainChannel)) {}

// TODO: strong type
void XmrManager::connect(const std::string& host)
{
    if (info_.host == host) return;

    info_.host = host;
    if (host.empty() || host == "DISABLED")
    {
        subscriber_.stop();
        Log::info("[XMR] Not configured");
        return;
    }

    auto endpoint = normalizeZmqEndpoint(host);
    Log::info("[XMR] Connecting to {}", endpoint);

    subscriber_.messageReceived().connect(
        [this](const Zmq::MultiPartMessage& message) { processMultipartMessage(message); });
    subscriber_.run(endpoint, Zmq::Channels{mainChannel_, HearbeatChannel});
}

void XmrManager::stop()
{
    subscriber_.stop();
}

CollectionIntervalAction& XmrManager::collectionInterval()
{
    return collectionIntervalAction_;
}

ScreenshotAction& XmrManager::screenshot()
{
    return screenshotAction_;
}

LayoutChangeAction& XmrManager::layoutChange()
{
    return layoutChangeAction_;
}

OverlayLayoutAction& XmrManager::overlayLayout()
{
    return overlayLayoutAction_;
}

RevertToScheduleAction& XmrManager::revertToSchedule()
{
    return revertToScheduleAction_;
}

CriteriaUpdateAction& XmrManager::criteriaUpdate()
{
    return criteriaUpdateAction_;
}

CommandAction& XmrManager::commandAction()
{
    return commandAction_;
}

DataUpdateAction& XmrManager::dataUpdate()
{
    return dataUpdateAction_;
}

TriggerWebhookAction& XmrManager::triggerWebhook()
{
    return triggerWebhookAction_;
}

PurgeAllAction& XmrManager::purgeAll()
{
    return purgeAllAction_;
}

XmrStatus XmrManager::status()
{
    return info_;
}

void XmrManager::processMultipartMessage(const Zmq::MultiPartMessage& multipart)
{
    if (multipart.empty())
    {
        Log::error("[XMR] Received empty message");
        return;
    }

    if (multipart[CHANNEL_PART] == mainChannel_)
    {
        if (multipart.size() <= MESSAGE_PART)
        {
            Log::error("[XMR] Received invalid message with {} parts", multipart.size());
            return;
        }

        try
        {
            auto decryptedMessage = decryptMessage(multipart[KEY_PART], multipart[MESSAGE_PART]);
            auto xmrMessage = parseMessage(decryptedMessage);

            processXmrMessage(xmrMessage);

            info_.lastMessageDt = DateTime::now();
        }
        catch (CryptoPP::Exception& e)
        {
            Log::error("[XMR::Crypto] {}. You need to reconfigure XMR for this display in the CMS and wait for the "
                       "next collection "
                       "interval so all keys will be updated.",
                       e.what());
        }
        catch (std::exception& e)
        {
            Log::error("[XMR] {}", e.what());
        }
    }
    else
    {
        info_.lastHeartbeatDt = DateTime::now();
    }
}

std::string XmrManager::decryptMessage(const std::string& encryptedBase64Key, const std::string& encryptedBase64Message)
{
    auto privateKey = RsaManager::instance().privateKey();

    auto encryptedKey = CryptoUtils::fromBase64(encryptedBase64Key);
    auto messageKey = CryptoUtils::decryptPrivateKeyPkcs(encryptedKey, privateKey);

    auto encryptedMessage = CryptoUtils::fromBase64(encryptedBase64Message);

    return CryptoUtils::decryptRc4(encryptedMessage, messageKey);
}

XmrMessage XmrManager::parseMessage(const std::string& jsonMessage)
{
    auto tree = Parsing::jsonFromString(jsonMessage);

    XmrMessage message;
    message.action = tree.get<std::string>("action");
    message.createdDt = parseCreatedDt(tree.get<std::string>("createdDt"));
    message.ttl = tree.get<int>("ttl");
    message.layoutId = tree.get("layoutId", EmptyLayoutId);
    message.duration = tree.get("duration", 0);
    message.downloadRequired = tree.get("downloadRequired", false);
    message.widgetId = tree.get("widgetId", 0);
    message.sourceId = tree.get("sourceId", 0);
    message.triggerCode = tree.get("triggerCode", std::string{});
    message.commandCode = tree.get("commandCode", std::string{});
    message.command = tree.get("command", std::string{});

    if (auto updates = tree.get_child_optional("criteriaUpdates"))
    {
        for (auto&& [_, item] : updates.value())
        {
            XmrMessage::CriteriaUpdate update;
            update.metric = item.get<std::string>("metric");
            update.value = item.get<std::string>("value");
            update.ttl = item.get("ttl", 300);
            message.criteriaUpdates.emplace_back(std::move(update));
        }
    }

    return message;
}

DateTime XmrManager::parseCreatedDt(const std::string& createdDt)
{
    std::string dateTimePart = createdDt;
    auto offset = DateTime::Seconds{0};

    if (boost::algorithm::ends_with(dateTimePart, "Z"))
    {
        dateTimePart.pop_back();
    }
    else if (dateTimePart.size() >= 25)
    {
        auto offsetStart = dateTimePart.find_last_of("+-");
        if (offsetStart != std::string::npos && offsetStart > 10)
        {
            auto sign = dateTimePart[offsetStart] == '-' ? -1 : 1;
            auto hours = std::stoi(dateTimePart.substr(offsetStart + 1, 2));
            auto minutes = std::stoi(dateTimePart.substr(offsetStart + 4, 2));

            offset = DateTime::Seconds{sign * ((hours * 60 + minutes) * 60)};
            dateTimePart = dateTimePart.substr(0, offsetStart);
        }
    }

    auto decimalStart = dateTimePart.find('.');
    if (decimalStart != std::string::npos)
    {
        dateTimePart = dateTimePart.substr(0, decimalStart);
    }

    auto parsed = DateTime::fromIsoExtendedString(dateTimePart);
    if (offset.total_seconds() > 0)
    {
        return parsed - offset;
    }

    return parsed + DateTime::Seconds{-offset.total_seconds()};
}

void XmrManager::processXmrMessage(const XmrMessage& message)
{
    Log::info("[XMR] Received action='{}' layoutId={} duration={} widgetId={} sourceId={} triggerCode='{}' "
              "commandCode='{}' commandLength={} downloadRequired={}",
              message.action,
              message.layoutId,
              message.duration,
              message.widgetId,
              message.sourceId,
              message.triggerCode,
              message.commandCode,
              message.command.size(),
              message.downloadRequired);

    if (isMessageExpired(message)) return;

    if (message.action == "collectNow" ||
        message.action == "rekeyAction" ||
        message.action == "licenceCheck" ||
        message.action == "clearStatsAndLogs")
    {
        MainLoop::pushToUiThread([this]() { collectionIntervalAction_(); });
    }
    else if (message.action == "screenShot")
    {
        MainLoop::pushToUiThread([this]() { screenshotAction_(); });
    }
    else if (message.action == "changeLayout")
    {
        MainLoop::pushToUiThread([this, message]() { layoutChangeAction_(message); });
    }
    else if (message.action == "overlayLayout")
    {
        MainLoop::pushToUiThread([this, message]() { overlayLayoutAction_(message); });
    }
    else if (message.action == "revertToSchedule")
    {
        MainLoop::pushToUiThread([this]() { revertToScheduleAction_(); });
    }
    else if (message.action == "criteriaUpdate")
    {
        MainLoop::pushToUiThread([this, message]() { criteriaUpdateAction_(message); });
    }
    else if (message.action == "commandAction")
    {
        MainLoop::pushToUiThread([this, message]() { commandAction_(message); });
    }
    else if (message.action == "dataUpdate")
    {
        MainLoop::pushToUiThread([this, message]() { dataUpdateAction_(message); });
    }
    else if (message.action == "triggerWebhook")
    {
        MainLoop::pushToUiThread([this, message]() { triggerWebhookAction_(message); });
    }
    else if (message.action == "purgeAll")
    {
        MainLoop::pushToUiThread([this]() { purgeAllAction_(); });
    }
    else
    {
        Log::info("[XMR] Unsupported action: {}", message.action);
    }
}

bool XmrManager::isMessageExpired(const XmrMessage& message)
{
    auto resultDt = message.createdDt + DateTime::Seconds(message.ttl);
    if (resultDt < DateTime::nowUtc())
    {
        return true;
    }
    return false;
}
