#pragma once

#include "common/dt/DateTime.hpp"
#include "schedule/ScheduleItem.hpp"
#include "xmr/XmrChannel.hpp"
#include "xmr/XmrStatus.hpp"
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
    std::vector<CriteriaUpdate> criteriaUpdates;
};

using CollectionIntervalAction = boost::signals2::signal<void()>;
using ScreenshotAction = boost::signals2::signal<void()>;
using LayoutChangeAction = boost::signals2::signal<void(const XmrMessage&)>;
using OverlayLayoutAction = boost::signals2::signal<void(const XmrMessage&)>;
using RevertToScheduleAction = boost::signals2::signal<void()>;
using CriteriaUpdateAction = boost::signals2::signal<void(const XmrMessage&)>;

class XmrManager
{
public:
    XmrManager(const XmrChannel& mainChannel);

    void connect(const std::string& host);
    void stop();

    CollectionIntervalAction& collectionInterval();
    ScreenshotAction& screenshot();
    LayoutChangeAction& layoutChange();
    OverlayLayoutAction& overlayLayout();
    RevertToScheduleAction& revertToSchedule();
    CriteriaUpdateAction& criteriaUpdate();
    XmrStatus status();

private:
    void processMultipartMessage(const Zmq::MultiPartMessage& message);
    std::string decryptMessage(const std::string& key, const std::string& message);
    XmrMessage parseMessage(const std::string& jsonMessage);
    DateTime parseCreatedDt(const std::string& createdDt);
    void processXmrMessage(const XmrMessage& message);
    bool isMessageExpired(const XmrMessage& message);

private:
    std::string mainChannel_;
    Zmq::Subscriber subscriber_;
    CollectionIntervalAction collectionIntervalAction_;
    ScreenshotAction screenshotAction_;
    LayoutChangeAction layoutChangeAction_;
    OverlayLayoutAction overlayLayoutAction_;
    RevertToScheduleAction revertToScheduleAction_;
    CriteriaUpdateAction criteriaUpdateAction_;
    XmrStatus info_;
};
