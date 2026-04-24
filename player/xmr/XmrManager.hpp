#pragma once

#include "common/dt/DateTime.hpp"
#include "schedule/ScheduleItem.hpp"
#include "xmr/XmrChannel.hpp"
#include "xmr/XmrStatus.hpp"
#include "xmr/ws/Subscriber.hpp"
#include "xmr/zmq/Subscriber.hpp"

#include <boost/signals2/signal.hpp>
#include <vector>

struct XmrMessage
{
    struct CriteriaUpdate
    {
        std::string metric;
        std::string value;
        int ttl = 300;
    };

    std::string action;
    DateTime createdDt;
    int ttl;
    int layoutId = EmptyLayoutId;
    int duration = 0;
    bool downloadRequired = false;
    int widgetId = 0;
    int sourceId = 0;
    std::string triggerCode;
    std::string commandCode;
    std::string command;
    std::vector<CriteriaUpdate> criteriaUpdates;
};

using CollectionIntervalAction = boost::signals2::signal<void()>;
using ScreenshotAction = boost::signals2::signal<void()>;
using LayoutChangeAction = boost::signals2::signal<void(const XmrMessage&)>;
using OverlayLayoutAction = boost::signals2::signal<void(const XmrMessage&)>;
using RevertToScheduleAction = boost::signals2::signal<void()>;
using CriteriaUpdateAction = boost::signals2::signal<void(const XmrMessage&)>;
using CommandAction = boost::signals2::signal<void(const XmrMessage&)>;
using DataUpdateAction = boost::signals2::signal<void(const XmrMessage&)>;
using TriggerWebhookAction = boost::signals2::signal<void(const XmrMessage&)>;
using PurgeAllAction = boost::signals2::signal<void()>;

class XmrManager
{
public:
    XmrManager(const XmrChannel& mainChannel);

    void connect(const std::string& host,
                 const std::string& xmrType,
                 const std::string& webSocketAddress,
                 const std::string& cmsKey,
                 const std::string& cmsAddress);
    void stop();

    CollectionIntervalAction& collectionInterval();
    ScreenshotAction& screenshot();
    LayoutChangeAction& layoutChange();
    OverlayLayoutAction& overlayLayout();
    RevertToScheduleAction& revertToSchedule();
    CriteriaUpdateAction& criteriaUpdate();
    CommandAction& commandAction();
    DataUpdateAction& dataUpdate();
    TriggerWebhookAction& triggerWebhook();
    PurgeAllAction& purgeAll();
    XmrStatus status();

private:
    void processWebSocketMessage(const std::string& message);
    void processMultipartMessage(const Zmq::MultiPartMessage& message);
    std::string decryptMessage(const std::string& key, const std::string& message);
    XmrMessage parseMessage(const std::string& jsonMessage);
    DateTime parseCreatedDt(const std::string& createdDt);
    void processXmrMessage(const XmrMessage& message);
    bool isMessageExpired(const XmrMessage& message);

private:
    std::string mainChannel_;
    std::string xmrHost_;
    std::string xmrType_;
    std::string webSocketAddress_;
    std::string cmsKey_;
    std::string cmsAddress_;
    bool usingWebSocket_ = false;
    Zmq::Subscriber subscriber_;
    Ws::Subscriber wsSubscriber_;
    CollectionIntervalAction collectionIntervalAction_;
    ScreenshotAction screenshotAction_;
    LayoutChangeAction layoutChangeAction_;
    OverlayLayoutAction overlayLayoutAction_;
    RevertToScheduleAction revertToScheduleAction_;
    CriteriaUpdateAction criteriaUpdateAction_;
    CommandAction commandAction_;
    DataUpdateAction dataUpdateAction_;
    TriggerWebhookAction triggerWebhookAction_;
    PurgeAllAction purgeAllAction_;
    XmrStatus info_;
};
