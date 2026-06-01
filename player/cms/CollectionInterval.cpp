#include "CollectionInterval.hpp"

#include "MainLoop.hpp"

#include "NotifyStatusInfo.hpp"
#include "common/PlayerRuntimeError.hpp"

#include <fmt/format.h>

// Add fmt formatter for std::atomic<int>
template<>
struct fmt::formatter<std::atomic<int>> : fmt::formatter<int> {
    auto format(const std::atomic<int>& atomic_val, format_context& ctx) const {
        return fmt::formatter<int>::format(atomic_val.load(), ctx);
    }
};
#include "common/dt/DateTime.hpp"
#include "common/dt/Timer.hpp"
#include "common/fs/FileSystem.hpp"
#include "common/fs/StorageUsageInfo.hpp"
#include "common/logger/Logging.hpp"
#include "common/logger/XmlLogsRetriever.hpp"
#include "common/storage/FileCache.hpp"
#include "common/system/System.hpp"
#include "config/AppConfig.hpp"

#include "cms/xmds/XmdsRequestSender.hpp"
#include "stat/Recorder.hpp"
#include "stat/records/XmlFormatter.hpp"

namespace ph = std::placeholders;

namespace
{
    std::size_t countSuccessfulDownloads(DownloadResults& results)
    {
        std::size_t successCount = 0;

        for (auto& result : results)
        {
            if (result.valid() && result.get())
            {
                ++successCount;
            }
        }

        return successCount;
    }
}

CollectionInterval::CollectionInterval(XmdsRequestSender& xmdsSender,
                                       Stats::Recorder& statsRecorder,
                                       FileCache& fileCache,
                                       const FilePath& resourceDirectory,
                                       const std::string& displayName) :
    xmdsSender_{xmdsSender},
    statsRecorder_{statsRecorder},
    fileCache_{fileCache},
    resourceDirectory_{resourceDirectory},
    displayName_{displayName},
    intervalTimer_{std::make_unique<Timer>()},
    collectInterval_{DefaultInterval},
    running_{false},
    status_{},
    currentLayoutId_{EmptyLayoutId}
{
    assert(intervalTimer_);
}

bool CollectionInterval::running() const
{
    return running_;
}

void CollectionInterval::stop()
{
    workerThread_.reset();
}

void CollectionInterval::startTimer()
{
    intervalTimer_->startOnce(std::chrono::seconds(collectInterval_), [this]() { collectNow(); });
}

void CollectionInterval::collectNow()
{
    if (!running_)
    {
        Log::info("[CollectionInterval] Starting collection cycle");
        running_ = true;
        workerThread_ = std::make_unique<JoinableThread>([=]() {
            Log::debug("[CollectionInterval] Started");

            auto registerDisplayResult =
                xmdsSender_.registerDisplay(std::stoi(AppConfig::codeVersion()), AppConfig::releaseVersion(), displayName_)
                    .get();
            onDisplayRegistered(registerDisplayResult);
        });
    }
}

void CollectionInterval::updateWidgetData(int widgetId)
{
    auto [error, result] = xmdsSender_.getData(widgetId).get();
    if (error)
    {
        Log::error("[CollectionInterval] Widget data update failed for widgetId={}: {}", widgetId, error);
        return;
    }

    WidgetDataFile widget{widgetId, 0};
    fileCache_.save(widget.name(), result.data, DateTime::nowUtc());
    Log::info("[CollectionInterval] Widget data update finished: widgetId={}, name={}", widgetId, widget.name());
}

void CollectionInterval::sessionFinished(const PlayerError& error)
{
    running_ = false;
    startTimer();
    Log::debug("[CollectionInterval] Finished. Next collection will start in {} seconds", collectInterval_);

    MainLoop::pushToUiThread([this, error]() { collectionFinished_(error); });
}

void CollectionInterval::onDisplayRegistered(const ResponseResult<RegisterDisplay::Result>& registerDisplay)
{
    auto [error, result] = registerDisplay;
    if (!error)
    {
        Log::info("[CollectionInterval] RegisterDisplay returned status code={}", static_cast<int>(result.status.code));
        auto displayError = displayStatus(result.status);
        if (!displayError)
        {
            Log::debug("[XMDS::RegisterDisplay] Success");
            Log::info("[CollectionInterval] Stage complete: registerDisplay");

            status_.registered = true;
            status_.lastChecked = DateTime::now();

            MainLoop::pushToUiThread([this, result = std::move(result.playerSettings)]() { settingsUpdated_(result); });

            auto requiredFilesResult = xmdsSender_.requiredFiles().get();
            auto scheduleResult = xmdsSender_.schedule().get();

            Log::info("[CollectionInterval] Stage start: requiredFiles");
            auto requiredFilesReady = onRequiredFiles(requiredFilesResult);
            Log::info("[CollectionInterval] Stage complete: requiredFiles");

            if (requiredFilesReady)
            {
                Log::info("[CollectionInterval] Stage start: schedule");
                onSchedule(scheduleResult);
                Log::info("[CollectionInterval] Stage complete: schedule");
            }
            else
            {
                Log::error("[CollectionInterval] Schedule update skipped because required files are not ready");
            }

            Log::info("[CollectionInterval] Stage start: submitLogs");
            submitLogs();
            Log::info("[CollectionInterval] Stage complete: submitLogs");

            Log::info("[CollectionInterval] Stage start: submitStats");
            submitStats();
            Log::info("[CollectionInterval] Stage complete: submitStats");

            Log::info("[CollectionInterval] Stage start: notifyStatus");
            notifyStatus();
            Log::info("[CollectionInterval] Stage complete: notifyStatus");
        }
        sessionFinished(displayError);
    }
    else
    {
        sessionFinished(error);
    }
}

void CollectionInterval::setCurrentLayoutId(const LayoutId& currentLayoutId)
{
    currentLayoutId_ = currentLayoutId;
}

PlayerError CollectionInterval::displayStatus(const RegisterDisplay::Result::Status& status)
{
    using DisplayCode = RegisterDisplay::Result::Status::Code;

    switch (status.code)
    {
        case DisplayCode::Ready: return {};
        case DisplayCode::Added:
        case DisplayCode::Waiting: return {"CMS", status.message};
        default: return {"CMS", "Unknown error with RegisterDisplay"};
    }
}

void CollectionInterval::updateInterval(int collectInterval)
{
    if (collectInterval_ != collectInterval)
    {
        Log::debug("[CollectionInterval] Interval updated to {} seconds", collectInterval);
        collectInterval_ = collectInterval;
    }
}

// TODO potential data race here
CmsStatus CollectionInterval::status() const
{
    return status_;
}

SignalSettingsUpdated& CollectionInterval::settingsUpdated()
{
    return settingsUpdated_;
}

SignalScheduleAvailable& CollectionInterval::scheduleAvailable()
{
    return scheduleAvailable_;
}

SignalCollectionFinished& CollectionInterval::collectionFinished()
{
    return collectionFinished_;
}

SignalFilesDownloaded& CollectionInterval::filesDownloaded()
{
    return filesDownloaded_;
}

bool CollectionInterval::onRequiredFiles(const ResponseResult<RequiredFiles::Result>& requiredFiles)
{
    auto [error, result] = requiredFiles;
    if (!error)
    {
        Log::debug("[XMDS::RequiredFiles] Received");

        RequiredFilesDownloader downloader{xmdsSender_, fileCache_};

        auto&& files = result.requiredFiles();
        auto&& resources = result.requiredResources();
        auto&& widgetData = result.requiredWidgetData();

        status_.requiredFiles = files.size() + resources.size() + widgetData.size();
        Log::info("[CollectionInterval] Required files received: regular={}, resources={}, widgetData={}, total={}",
                  files.size(),
                  resources.size(),
                  widgetData.size(),
                  status_.requiredFiles);

        auto resourcesResult = downloader.download(resources);
        auto filesResult = downloader.download(files);
        auto widgetDataResult = downloader.download(widgetData);

        auto resourceDownloads = resourcesResult.get();
        auto fileDownloads = filesResult.get();
        auto widgetDownloads = widgetDataResult.get();

        auto downloadedResources = countSuccessfulDownloads(resourceDownloads);
        auto downloadedFiles = countSuccessfulDownloads(fileDownloads);
        auto downloadedWidgets = countSuccessfulDownloads(widgetDownloads);

        Log::info("[CollectionInterval] Download batch finished: regular ok={}/{}, resources ok={}/{}, widgetData ok={}/{}",
                  downloadedFiles,
                  fileDownloads.size(),
                  downloadedResources,
                  resourceDownloads.size(),
                  downloadedWidgets,
                  widgetDownloads.size());

        updateMediaInventory(result);
        Log::info("[CollectionInterval] Media inventory updated after downloads");

        MainLoop::pushToUiThread([this]() { filesDownloaded_(); });
        return downloadedFiles == fileDownloads.size()
            && downloadedResources == resourceDownloads.size();
    }
    else
    {
        sessionFinished(error);
        return false;
    }
}

void CollectionInterval::updateMediaInventory(const RequiredFiles::Result& result)
{
    MediaInventoryItems items;
    for (auto&& file : result.requiredFiles())
    {
        items.emplace_back(file, fileCache_.valid(file.name()));
    }
    for (auto&& file : result.requiredResources())
    {
        items.emplace_back(file, fileCache_.valid(file.name()));
    }
    onSubmitted("MediaInventory", xmdsSender_.mediaInventory(std::move(items)).get());
}

void CollectionInterval::onSchedule(const ResponseResult<Schedule::Result>& schedule)
{
    auto [error, result] = schedule;
    if (!error)
    {
        Log::debug("[XMDS::Schedule] Received");
        Log::info("[CollectionInterval] Schedule XML received: {} bytes", result.scheduleXml.size());
        MainLoop::pushToUiThread([this, result = std::move(result)]() {
            scheduleAvailable_(LayoutSchedule::fromString(result.scheduleXml));
        });
    }
    else
    {
        sessionFinished(error);
    }
}

void CollectionInterval::submitLogs()
{
    XmlLogsRetriever logsRetriever;
    auto submitLogsResult = xmdsSender_.submitLogs(logsRetriever.retrieveLogs()).get();
    onSubmitted("SubmitLogs", submitLogsResult);
}

void CollectionInterval::submitStats()
{
    try
    {
        const auto recordsCount = statsRecorder_.recordsCount();
        if (recordsCount > 0)
        {
            const auto RecordsToSend = [recordsCount]() -> size_t {
                if (recordsCount > 500)
                    return 300;
                else
                    return recordsCount > 50 ? 50 : recordsCount;
            }();

            Log::debug("[CollectionInterval] Total records: {} Records to send {}", recordsCount, RecordsToSend);

            auto records = statsRecorder_.records(RecordsToSend);
            statsRecorder_.removeFromQueue(RecordsToSend);

            Stats::XmlFormatter formatter;
            auto submitStatsResult = xmdsSender_.submitStats(formatter.format(records)).get();
            onSubmitted("SubmitStats", submitStatsResult);
        }
    }
    catch (const std::exception& e)
    {
        Log::error("[CollectionInterval] {}", e.what());
        Log::error("[CollectionInterval] Failed to submit stats");
    }
}

void CollectionInterval::notifyStatus()
{
    NotifyStatusInfo notifyInfo;
    notifyInfo.deviceName = System::hostname();

    try
    {
        notifyInfo.spaceUsageInfo = FileSystem::storageUsageFor(resourceDirectory_);
        notifyInfo.hasSpaceUsageInfo = true;
    }
    catch (const std::exception& e)
    {
        Log::debug("[XMDS::NotifyStatus] Unable to get storage usage: {}", e.what());
    }

    try
    {
        notifyInfo.timezone = DateTime::currentTimezone();
    }
    catch (const std::exception& e)
    {
        Log::debug("[XMDS::NotifyStatus] Unable to get timezone: {}", e.what());
    }

    auto status = notifyInfo.string();
    Log::debug("[XMDS::NotifyStatus] Payload: {}", status);

    auto notifyStatusResult = xmdsSender_.notifyStatus(status).get();
    auto [error, result] = notifyStatusResult;
    if (error)
    {
        Log::error("[XMDS::NotifyStatus] Payload failed: {}", status);
    }

    onSubmitted("NotifyStatus", notifyStatusResult);
}

template <typename Result>
void CollectionInterval::onSubmitted(std::string_view requestName, const ResponseResult<Result>& submitResult)
{
    auto [error, result] = submitResult;
    if (!error)
    {
        if (result.success)
        {
            Log::debug("[XMDS::{}] Submitted", requestName);
        }
        else
        {
            Log::error("[XMDS::{}] Not submited due to unknown error", requestName);
        }
    }
    else
    {
        Log::error("[XMDS::{}] Submit failed: {}", requestName, error);
        sessionFinished(error);
    }
}
