#include "Scheduler.hpp"

#include "control/cache/UnsafeItemStore.hpp"
#include "common/dt/DateTime.hpp"
#include "common/logger/Logging.hpp"

#include <algorithm>

Scheduler::Scheduler(const FileCache& fileCache) : fileCache_{fileCache}, schedule_{} {}

void Scheduler::reloadSchedule(LayoutSchedule&& schedule)
{
    if (!schedule_.has_value() || schedule_ != schedule)
    {
        schedule_ = std::move(schedule);
        scheduleUpdated_(schedule_.value());

        reloadQueue();
    }
}

void Scheduler::reloadQueue()
{
    assert(schedule_);

    auto current = layoutOverride_ ? layoutOverride_->layout.id : regularQueue_.current();
    auto overlays = overlayQueue_.overlays();
    for (auto&& layout : overlayOverrides_)
    {
        if (std::find(overlays.begin(), overlays.end(), layout.id) == overlays.end())
        {
            overlays.push_back(layout.id);
        }
    }

    cleanupExpiredOverrides();
    cleanupExpiredCriteria();

    regularQueue_ = regularQueueFrom(schedule_.value());
    overlayQueue_ = overlayQueueFrom(schedule_.value());
    restartTimer();

    updateCurrentLayout(current);
    updateCurrentOverlays(overlays);
}

void Scheduler::applyLayoutOverride(LayoutId id, const DateTime& createdDt, int duration)
{
    cleanupExpiredOverrides();

    LayoutOverride overrideState;
    overrideState.layout = ScheduledLayout{DefaultScheduleId, id, 0, createdDt - DateTime::Seconds(1), {}, {}, {}};
    overrideState.oneShot = duration == 0;
    overrideState.played = false;

    if (!overrideState.oneShot)
    {
        overrideState.layout.endDT = createdDt + DateTime::Seconds(duration);
    }

    layoutOverride_ = std::move(overrideState);
    if (schedule_) restartTimer();
    layoutUpdated_();
}

void Scheduler::addOverlayOverride(LayoutId id, const DateTime& createdDt, int duration)
{
    cleanupExpiredOverrides();

    overlayOverrides_.erase(
        std::remove_if(overlayOverrides_.begin(),
                       overlayOverrides_.end(),
                       [id](const ScheduledLayout& layout) { return layout.id == id; }),
        overlayOverrides_.end());

    overlayOverrides_.push_back(
        ScheduledLayout{DefaultScheduleId, id, 0, createdDt, createdDt + DateTime::Seconds(duration), {}, {}});
    if (schedule_) restartTimer();
    overlaysUpdated_();
}

void Scheduler::clearOverrides()
{
    cleanupExpiredOverrides();

    auto hadLayoutOverride = layoutOverride_.has_value();
    auto hadOverlayOverrides = !overlayOverrides_.empty();

    layoutOverride_ = {};
    overlayOverrides_.clear();
    if (schedule_) restartTimer();

    if (hadLayoutOverride)
    {
        layoutUpdated_();
    }

    if (hadOverlayOverrides)
    {
        overlaysUpdated_();
    }
}

void Scheduler::triggerNextLayout()
{
    cleanupExpiredOverrides();
    if (cleanupExpiredCriteria() && schedule_)
    {
        reloadQueue();
    }

    if (!schedule_) return;

    auto nextId = regularQueue_.next();
    if (nextId == EmptyLayoutId)
    {
        Log::error("[Scheduler] Trigger next requested, but no valid layout is available");
        return;
    }

    Log::info("[Scheduler] Trigger next resolved to layout {}", nextId);
    applyLayoutOverride(nextId, DateTime::now(), 0);
}

void Scheduler::triggerPreviousLayout()
{
    cleanupExpiredOverrides();
    if (cleanupExpiredCriteria() && schedule_)
    {
        reloadQueue();
    }

    if (!schedule_) return;

    auto previousId = regularQueue_.previous();
    if (previousId == EmptyLayoutId)
    {
        Log::error("[Scheduler] Trigger previous requested, but no valid layout is available");
        return;
    }

    Log::info("[Scheduler] Trigger previous resolved to layout {}", previousId);
    applyLayoutOverride(previousId, DateTime::now(), 0);
}

bool Scheduler::triggerLayoutById(LayoutId id)
{
    cleanupExpiredOverrides();
    if (cleanupExpiredCriteria() && schedule_)
    {
        reloadQueue();
    }

    if (!schedule_) return false;

    if (!layoutById(id))
    {
        Log::error("[Scheduler] Trigger navLayout ignored: layout {} is not in current schedule", id);
        return false;
    }

    Log::info("[Scheduler] Trigger navLayout resolved to layout {}", id);
    applyLayoutOverride(id, DateTime::now(), 0);
    return true;
}

void Scheduler::addOrReplaceCriteria(const std::string& metric, const std::string& value, int ttl)
{
    criteria_[metric] = ActiveCriteria{value, DateTime::now() + DateTime::Seconds(ttl)};

    if (schedule_)
    {
        reloadQueue();
    }
}

RegularLayoutQueue Scheduler::regularQueueFrom(const LayoutSchedule& schedule)
{
    RegularLayoutQueue queue;

    for (auto&& layout : schedule.regularLayouts)
    {
        if (layoutOnSchedule(layout) && layoutValid(layout) && layoutCriteriaActive(layout))
        {
            queue.add(layout);
        }
    }

    if (layoutValid(schedule.defaultLayout))
    {
        queue.addDefault(schedule.defaultLayout);
    }

    return queue;
}

OverlayLayoutQueue Scheduler::overlayQueueFrom(const LayoutSchedule& schedule)
{
    OverlayLayoutQueue queue;

    for (auto&& layout : schedule.overlayLayouts)
    {
        if (layoutOnSchedule(layout) && layoutValid(layout) && layoutCriteriaActive(layout))
        {
            queue.add(layout);
        }
    }

    return queue;
}

void Scheduler::updateCurrentLayout(LayoutId id)
{
    if (regularQueue_.inQueue(id))
    {
        regularQueue_.updateCurrent(id);
    }
    else
    {
        layoutUpdated_();
    }
}

void Scheduler::updateCurrentOverlays(const OverlaysIds& ids)
{
    if (overlayQueue_.overlays() != ids)
    {
        overlaysUpdated_();
    }
}

bool Scheduler::layoutOnSchedule(const ScheduledLayout& layout) const
{
    auto currentDT = DateTime::now();

    if (currentDT >= layout.startDT && currentDT < layout.endDT)
    {
        return true;
    }

    return false;
}

template <typename Layout>
bool Scheduler::layoutValid(const Layout& layout) const
{
    assert(schedule_);

    if (UnsafeItemStore::instance().isUnsafeLayout(layout.id))
    {
        Log::error("[Scheduler] Layout {} invalid: flagged as unsafe", layout.id);
        return false;
    }

    auto layoutFile = std::to_string(layout.id) + ".xlf";
    if (!fileCache_.valid(layoutFile))
    {
        Log::error("[Scheduler] Layout {} invalid: '{}' is not valid in cache", layout.id, layoutFile);
        return false;
    }

    for (auto&& dependant : layout.dependants)
    {
        if (!fileCache_.valid(dependant))
        {
            Log::error("[Scheduler] Layout {} invalid: dependant '{}' is not valid in cache", layout.id, dependant);
            return false;
        }
    }

    for (auto&& dependant : schedule_->globalDependants)
    {
        if (!fileCache_.valid(dependant))
        {
            Log::error(
                "[Scheduler] Layout {} invalid: global dependant '{}' is not valid in cache", layout.id, dependant);
            return false;
        }
    }

    return true;
}

LayoutId Scheduler::nextLayout() const
{
    const_cast<Scheduler*>(this)->cleanupExpiredOverrides();
    if (const_cast<Scheduler*>(this)->cleanupExpiredCriteria() && schedule_)
    {
        const_cast<Scheduler*>(this)->reloadQueue();
    }

    if (hasActiveLayoutOverride() && layoutValid(layoutOverride_->layout))
    {
        layoutOverride_->played = true;
        return layoutOverride_->layout.id;
    }

    return regularQueue_.next();
}

LayoutId Scheduler::currentLayoutId() const
{
    const_cast<Scheduler*>(this)->cleanupExpiredOverrides();
    if (const_cast<Scheduler*>(this)->cleanupExpiredCriteria() && schedule_)
    {
        const_cast<Scheduler*>(this)->reloadQueue();
    }

    if (hasActiveLayoutOverride())
    {
        return layoutOverride_->layout.id;
    }

    return regularQueue_.current();
}

OverlaysIds Scheduler::overlayLayouts() const
{
    const_cast<Scheduler*>(this)->cleanupExpiredOverrides();
    if (const_cast<Scheduler*>(this)->cleanupExpiredCriteria() && schedule_)
    {
        const_cast<Scheduler*>(this)->reloadQueue();
    }

    auto overlays = overlayQueue_.overlays();

    for (auto&& layout : overlayOverrides_)
    {
        if (layoutValid(layout) && std::find(overlays.begin(), overlays.end(), layout.id) == overlays.end())
        {
            overlays.push_back(layout.id);
        }
    }

    return overlays;
}

SignalScheduleUpdated& Scheduler::scheduleUpdated()
{
    return scheduleUpdated_;
}

SignalLayoutsUpdated& Scheduler::overlaysUpdated()
{
    return overlaysUpdated_;
}

DateTime Scheduler::closestLayoutDt()
{
    assert(schedule_);

    auto now = DateTime::now();
    DateTime closestDt;

    for (auto&& layout : schedule_->regularLayouts)
    {
        if (now < layout.startDT && layout.startDT < closestDt) closestDt = layout.startDT;
        if (now < layout.endDT && layout.endDT < closestDt) closestDt = layout.endDT;
    }

    for (auto&& layout : schedule_->overlayLayouts)
    {
        if (now < layout.startDT && layout.startDT < closestDt) closestDt = layout.startDT;
        if (now < layout.endDT && layout.endDT < closestDt) closestDt = layout.endDT;
    }

    if (layoutOverride_ && !layoutOverride_->oneShot && now < layoutOverride_->layout.endDT &&
        layoutOverride_->layout.endDT < closestDt)
    {
        closestDt = layoutOverride_->layout.endDT;
    }

    for (auto&& layout : overlayOverrides_)
    {
        if (now < layout.endDT && layout.endDT < closestDt) closestDt = layout.endDT;
    }

    return closestDt;
}

void Scheduler::restartTimer()
{
    if (!schedule_) return;

    auto dt = closestLayoutDt();
    auto duration = (dt - DateTime::now()).total_seconds();

    if (dt.valid() && duration > 0)
    {
        Log::trace("[Scheduler] Timer restarted: {}", duration);

        timer_.startOnce(std::chrono::seconds(duration), std::bind(&Scheduler::reloadQueue, this));
    }
}

SignalLayoutsUpdated& Scheduler::layoutUpdated()
{
    return layoutUpdated_;
}

SchedulerStatus Scheduler::status() const
{
    assert(schedule_);
    if (const_cast<Scheduler*>(this)->cleanupExpiredCriteria())
    {
        const_cast<Scheduler*>(this)->reloadQueue();
    }

    SchedulerStatus status;

    fillSchedulerStatus(status, schedule_->regularLayouts);
    fillSchedulerStatus(status, schedule_->overlayLayouts);
    addDefaultToStatus(status, schedule_->defaultLayout);

    status.generatedTime = schedule_->generatedTime.string();
    status.currentLayout = currentLayoutId();
    status.weatherCriteriaActive = isWeatherCriteriaActive();
    for (auto&& [metric, criteria] : criteria_)
    {
        status.activeCriteria.emplace_back(metric + ": " + criteria.value);
    }

    return status;
}

int Scheduler::scheduleIdBy(LayoutId id) const
{
    assert(schedule_);

    auto layout = layoutById(id);
    if (layout)
    {
        return layout->scheduleId;
    }
    return DefaultScheduleId;
}

boost::optional<ScheduledLayout> Scheduler::layoutById(int id) const
{
    assert(schedule_);

    if (layoutOverride_ && layoutOverride_->layout.id == id)
    {
        return layoutOverride_->layout;
    }

    {
        auto&& regularLayouts = schedule_->regularLayouts;
        auto it = std::find_if(regularLayouts.begin(), regularLayouts.end(), [id](const ScheduledLayout& other) {
            return other.id == id;
        });

        if (it != regularLayouts.end()) return *it;
    }
    {
        auto&& overlayLayouts = schedule_->overlayLayouts;
        auto it = std::find_if(overlayLayouts.begin(), overlayLayouts.end(), [id](const ScheduledLayout& other) {
            return other.id == id;
        });

        if (it != overlayLayouts.end()) return *it;
    }

    return {};
}

void Scheduler::cleanupExpiredOverrides()
{
    auto now = DateTime::now();

    if (layoutOverride_)
    {
        auto expired = (!layoutOverride_->oneShot && layoutOverride_->layout.endDT <= now) ||
            (layoutOverride_->oneShot && layoutOverride_->played);
        if (expired)
        {
            layoutOverride_ = {};
        }
    }

    overlayOverrides_.erase(std::remove_if(overlayOverrides_.begin(),
                                           overlayOverrides_.end(),
                                           [now](const ScheduledLayout& layout) { return layout.endDT <= now; }),
                            overlayOverrides_.end());
}

bool Scheduler::cleanupExpiredCriteria()
{
    auto now = DateTime::now();
    auto removed = false;

    for (auto it = criteria_.begin(); it != criteria_.end();)
    {
        if (it->second.expiresAt <= now)
        {
            it = criteria_.erase(it);
            removed = true;
        }
        else
        {
            ++it;
        }
    }

    return removed;
}

bool Scheduler::hasActiveLayoutOverride() const
{
    return layoutOverride_.has_value();
}

bool Scheduler::hasActiveOverlayOverrides() const
{
    return !overlayOverrides_.empty();
}

bool Scheduler::layoutCriteriaActive(const ScheduledLayout& layout) const
{
    for (auto&& criteria : layout.criterias)
    {
        if (!criteriaActive(criteria))
        {
            return false;
        }
    }

    return true;
}

bool Scheduler::criteriaActive(const ScheduleCriteria& scheduleCriteria) const
{
    auto it = criteria_.find(scheduleCriteria.metric);
    if (it == criteria_.end())
    {
        return false;
    }

    const auto& criteria = it->second;
    if (criteria.expiresAt <= DateTime::now())
    {
        return false;
    }

    if (scheduleCriteria.condition == "set") return true;
    if (scheduleCriteria.condition == "eq") return scheduleCriteria.value == criteria.value;
    if (scheduleCriteria.condition == "neq") return scheduleCriteria.value != criteria.value;
    if (scheduleCriteria.condition == "contains") return scheduleCriteria.value.find(criteria.value) != std::string::npos;
    if (scheduleCriteria.condition == "ncontains") return scheduleCriteria.value.find(criteria.value) == std::string::npos;

    try
    {
        auto criteriaValue = std::stoi(criteria.value);
        auto scheduleValue = std::stoi(scheduleCriteria.value);

        if (scheduleCriteria.condition == "lt") return criteriaValue < scheduleValue;
        if (scheduleCriteria.condition == "lte") return criteriaValue <= scheduleValue;
        if (scheduleCriteria.condition == "gte") return criteriaValue >= scheduleValue;
        if (scheduleCriteria.condition == "gt") return criteriaValue > scheduleValue;
    }
    catch (const std::exception&)
    {
        return false;
    }

    return false;
}

bool Scheduler::isWeatherCriteriaActive() const
{
    if (!schedule_) return false;

    auto hasWeatherCriteria = [](const LayoutList& layouts) {
        return std::any_of(layouts.begin(), layouts.end(), [](const ScheduledLayout& layout) {
            return std::any_of(layout.criterias.begin(), layout.criterias.end(), [](const ScheduleCriteria& criteria) {
                return criteria.type == "weather";
            });
        });
    };

    return hasWeatherCriteria(schedule_->regularLayouts) || hasWeatherCriteria(schedule_->overlayLayouts);
}

template <typename LayoutsList>
void Scheduler::fillSchedulerStatus(SchedulerStatus& status, const LayoutsList& layouts) const
{
    for (auto&& layout : layouts)
    {
        if (layoutValid(layout))
        {
            status.validLayouts.emplace_back(layout.id);
            if (layoutOnSchedule(layout) && layoutCriteriaActive(layout))
            {
                status.scheduledLayouts.emplace_back(layout.id);
            }
        }
        else
        {
            status.invalidLayouts.emplace_back(layout.id);
        }
    }
}

void Scheduler::addDefaultToStatus(SchedulerStatus& status, const DefaultScheduledLayout& layout) const
{
    if (layoutValid(layout))
    {
        status.validLayouts.emplace_back(layout.id);
    }
    else
    {
        status.invalidLayouts.emplace_back(layout.id);
    }
}
