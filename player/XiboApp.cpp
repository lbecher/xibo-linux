#include "XiboApp.hpp"
#include "MainLoop.hpp"

#include "config/AppConfig.hpp"
#include "config/CmsSettings.hpp"
#include "config/PlayerSettings.hpp"

#include "control/ApplicationWindow.hpp"
#include "control/cache/UnsafeItemStore.hpp"
#include "control/layout/LayoutsManager.hpp"
#include "control/media/MediaParsersRepo.hpp"
#include "control/server/EmbeddedServer.hpp"
#include "control/screenshot/ScreeShoterFactory.hpp"
#include "control/screenshot/ScreenShotInterval.hpp"

#include "schedule/Scheduler.hpp"
#include "stat/Recorder.hpp"
#include "xmr/XmrManager.hpp"

#include "cms/CollectionInterval.hpp"
#include "cms/xmds/SoapRequestSender.hpp"
#include "cms/xmds/XmdsRequestSender.hpp"
#include "networking/HttpClient.hpp"

#include "common/PlayerRuntimeError.hpp"
#include "common/crypto/RsaManager.hpp"
#include "common/fs/FileSystem.hpp"
#include "common/logger/Logging.hpp"
#include "common/parsing/Parsing.hpp"
#include "common/storage/FileCacheImpl.hpp"
#include "common/system/System.hpp"

static std::unique_ptr<XiboApp> g_app;

XiboApp& xiboApp()
{
    return *g_app;
}

XiboApp& XiboApp::create(const std::string& name)
{
    Log::create();

    g_app = std::unique_ptr<XiboApp>(new XiboApp(name));
    return *g_app;
}

XiboApp::XiboApp(const std::string& name) :
    mainLoop_(std::make_unique<MainLoop>(name)),
    fileCache_(std::make_unique<FileCacheImpl>()),
    scheduler_(std::make_unique<Scheduler>(*fileCache_)),
    statsRecorder_(std::make_unique<Stats::Recorder>()),
    webserver_(std::make_shared<EmbeddedServer>())
{
    Log::info("[XiboApp] Startup stage: constructing application");
    if (!FileSystem::exists(AppConfig::cmsSettingsPath()))
        throw PlayerRuntimeError{"XiboApp", "Update CMS settings using player options app"};

    playerSettings_.logLevel().valueChanged().connect([](const std::string& logLevel) { Log::setLevel(logLevel); });

    cmsSettings_.fromFile(AppConfig::cmsSettingsPath());
    playerSettings_.fromFile(AppConfig::playerSettingsPath());
    fileCache_->loadFrom(AppConfig::cachePath());
    Log::info("[XiboApp] Startup stage: settings and cache loaded");

    System::preventSleep();
    AppConfig::resourceDirectory(cmsSettings_.resourcesPath());
    checkResourceDirectory();
    Log::info("[XiboApp] Startup stage: resources checked at {}", cmsSettings_.resourcesPath().value().string());

    webserver_->setRootDirectory(cmsSettings_.resourcesPath());
    webserver_->setInfoFactory([this]() {
        JsonNode root;
        auto general = collectGeneralInfo();
        auto scheduler = scheduler_->status();
        auto xmr = xmrManager_ ? xmrManager_->status() : XmrStatus{};

        root.put("hardwareKey", static_cast<const std::string&>(static_cast<const Md5Hash&>(System::hardwareKey())));
        root.put("displayName", general.displayName);
        root.put("timeZone", DateTime::currentTimezone());
        root.put("cmsAddress", general.cmsAddress);
        root.put("screenShotInterval", general.screenShotInterval);
        root.put("currentLayout", scheduler.currentLayout);
        root.put("generatedTime", scheduler.generatedTime);
        root.put("controlCount", controlCount_);
        root.put("lastTriggerCode", lastTriggerCode_);
        root.put("lastTriggerSourceId", lastTriggerSourceId_);
        root.put("lastDurationOperation", lastDurationOperation_);
        root.put("lastDurationSourceId", lastDurationSourceId_);
        root.put("lastDuration", lastDuration_);
        root.put("lastFaultCode", lastFaultCode_);
        root.put("lastFaultKey", lastFaultKey_);
        root.put("lastFaultTtl", lastFaultTtl_);
        root.put("lastFaultReason", lastFaultReason_);
        root.put("unsafeList", UnsafeItemStore::instance().listAsString());
        root.put("unsafeListJson", UnsafeItemStore::instance().listAsJsonString());
        root.put("xmr.host", xmr.host);
        root.put("xmr.lastHeartbeat", xmr.lastHeartbeatDt.string());
        root.put("xmr.lastMessage", xmr.lastMessageDt.string());

        JsonNode criteria;
        for (auto&& value : scheduler.activeCriteria)
        {
            JsonNode item;
            item.put("", value);
            criteria.push_back(std::make_pair("", item));
        }
        root.add_child("criteria", criteria);

        return Parsing::jsonToString(root);
    });
    webserver_->setCriteriaReceived([this](const CriteriaRequests& updates) {
        MainLoop::pushToUiThread([this, updates]() {
            for (auto&& update : updates)
            {
                scheduler_->addOrReplaceCriteria(update.metric, update.value, update.ttl);
            }
        });
    });
    webserver_->setTriggerReceived([this](const TriggerRequest& request) {
        MainLoop::pushToUiThread([this, request]() {
            ++controlCount_;
            lastTriggerCode_ = request.trigger;
            lastTriggerSourceId_ = request.id;

            Log::info("[Control] Trigger received: code={}, sourceId={}", request.trigger, request.id);
        });
    });
    webserver_->setDurationReceived([this](const DurationRequest& request) {
        MainLoop::pushToUiThread([this, request]() {
            ++controlCount_;
            lastDurationOperation_ = request.operation;
            lastDurationSourceId_ = request.id;
            lastDuration_ = request.duration;

            Log::info("[Control] Duration request received: operation={}, sourceId={}, duration={}",
                      request.operation,
                      request.id,
                      request.duration);
        });
    });
    webserver_->setFaultReceived([this](const FaultRequest& request) {
        MainLoop::pushToUiThread([this, request]() {
            ++controlCount_;
            lastFaultCode_ = request.code;
            lastFaultKey_ = request.key;
            lastFaultTtl_ = request.ttl;
            lastFaultReason_ = request.reason;

            auto widgetSeparator = request.key.find('_');
            if (widgetSeparator != std::string::npos && widgetSeparator + 1 < request.key.size())
            {
                UnsafeItemStore::instance().addUnsafeWidget(
                    request.code, request.key.substr(widgetSeparator + 1), request.reason, request.ttl);
            }

            Log::info("[Control] Fault request received: code={}, key={}, ttl={}, reason={}",
                      request.code,
                      request.key,
                      request.ttl,
                      request.reason);
        });
    });
    webserver_->run(playerSettings_.embeddedServerPort());
    Log::info("[XiboApp] Startup stage: embedded server listening on {}",
              playerSettings_.embeddedServerPort().value());

    HttpClient::instance().setProxyServer(cmsSettings_.proxy());
    RsaManager::instance().load();
    xmrManager_ = createXmrManager();

    MediaParsersRepo::init();
    Log::info("[XiboApp] Startup stage: XMR and media parsers initialized");

    mainLoop_->setShutdownAction([this]() {
        layoutManager_.reset();
        xmrManager_->stop();
        HttpClient::instance().shutdown();
        if (collectionInterval_)
        {
            collectionInterval_->stop();
        }
    });
}

std::unique_ptr<XmrManager> XiboApp::createXmrManager()
{
    auto xmrChannel = XmrChannel::fromCmsSettings(cmsSettings_.address(), cmsSettings_.key(), cmsSettings_.displayId());
    auto manager = std::make_unique<XmrManager>(xmrChannel);

    playerSettings_.xmrNetworkAddress().valueChanged().connect(std::bind(&XmrManager::connect, manager.get(), ph::_1));

    manager->collectionInterval().connect([this]() {
        CHECK_UI_THREAD();
        Log::info("[XMR] Start unscheduled collection");

        collectionInterval_->collectNow();
    });

    manager->screenshot().connect([this]() {
        CHECK_UI_THREAD();
        Log::info("[XMR] Taking unscheduled screenshot");

        screenShotInterval_->takeScreenShot();
    });

    manager->layoutChange().connect([this](const XmrMessage& message) {
        CHECK_UI_THREAD();
        Log::info("[XMR] Applying temporary layout override: {}", message.layoutId);

        if (message.downloadRequired)
        {
            collectionInterval_->collectNow();
        }

        scheduler_->applyLayoutOverride(message.layoutId, message.createdDt, message.duration);
    });

    manager->overlayLayout().connect([this](const XmrMessage& message) {
        CHECK_UI_THREAD();
        Log::info("[XMR] Applying temporary overlay layout: {}", message.layoutId);

        if (message.downloadRequired)
        {
            collectionInterval_->collectNow();
        }

        scheduler_->addOverlayOverride(message.layoutId, message.createdDt, message.duration);
    });

    manager->revertToSchedule().connect([this]() {
        CHECK_UI_THREAD();
        Log::info("[XMR] Reverting to scheduled content");

        scheduler_->clearOverrides();
    });

    manager->criteriaUpdate().connect([this](const XmrMessage& message) {
        CHECK_UI_THREAD();
        Log::info("[XMR] Applying {} criteria updates", message.criteriaUpdates.size());

        for (auto&& update : message.criteriaUpdates)
        {
            scheduler_->addOrReplaceCriteria(update.metric, update.value, update.ttl);
        }
    });

    return manager;
}

int XiboApp::run()
{
    Log::info("[XiboApp] Run stage: creating main window");
    mainWindow_ = createMainWindow();
    Log::info("[XiboApp] Run stage: creating layout manager");
    layoutManager_ = createLayoutManager();

    playerSettings_.statsEnabled().valueChanged().connect(
        [this](bool statsEnabled) { layoutManager_->statsEnabled(statsEnabled); });

    xmdsManager_ =
        std::make_unique<XmdsRequestSender>(cmsSettings_.address(), cmsSettings_.key(), cmsSettings_.displayId());
    screenShotInterval_ = createScreenshotInterval(*xmdsManager_, *mainWindow_);
    collectionInterval_ = createCollectionInterval(*xmdsManager_);
    Log::info("[XiboApp] Run stage: screenshot and collection intervals created");

    collectionInterval_->setCurrentLayoutId(scheduler_->currentLayoutId());

    scheduler_->layoutUpdated().connect(
        [this]() {
            auto currentLayoutId = scheduler_->currentLayoutId();
            Log::info("[XiboApp] Scheduler layoutUpdated: currentLayoutId={}", currentLayoutId);
            collectionInterval_->setCurrentLayoutId(currentLayoutId);
        });

    Log::info("[XiboApp] Run stage: loading cached schedule from {}", AppConfig::schedulePath().string());
    scheduler_->reloadSchedule(LayoutSchedule::fromFile(AppConfig::schedulePath()));
    scheduler_->scheduleUpdated().connect(
        [](const LayoutSchedule& schedule) {
            Log::info("[XiboApp] Scheduler emitted scheduleUpdated: regularLayouts={}, overlayLayouts={}",
                      schedule.regularLayouts.size(),
                      schedule.overlayLayouts.size());
            schedule.toFile(AppConfig::schedulePath());
        });

    mainLoop_->started().connect([this] {
        Log::info("[XiboApp] Main loop started: triggering initial collection");
        collectionInterval_->collectNow();
        mainWindow_->showAll();
    });

    return mainLoop_->run(*mainWindow_);
}

Uri XiboApp::localAddress()
{
    return g_app->webserver_->address();
}

std::shared_ptr<ApplicationWindowGtk> XiboApp::createMainWindow()
{
    std::shared_ptr<ApplicationWindowGtk> window = ApplicationWindowGtk::create(playerSettings_.size().width(),
                                                                                playerSettings_.size().height(),
                                                                                playerSettings_.position().x(),
                                                                                playerSettings_.position().y());

    playerSettings_.size().valueChanged().connect([window](int width, int height) { window->setSize(width, height); });
    playerSettings_.position().valueChanged().connect([window](int x, int y) { window->move(x, y); });

    window->statusScreenShown().connect([this, window]() {
        CHECK_UI_THREAD();
        StatusInfo info{
            collectGeneralInfo(), collectionInterval_->status(), scheduler_->status(), xmrManager_->status()};

        window->updateStatusScreen(info, fileCache_->invalidFiles());
    });

    window->exitWithoutRestartRequested().connect([]() { System::terminateProccess(System::parentProcessId()); });

    return window;
}

std::unique_ptr<LayoutsManager> XiboApp::createLayoutManager()
{
    auto manager =
        std::make_unique<LayoutsManager>(*scheduler_, *statsRecorder_, *fileCache_, playerSettings_.statsEnabled());

    manager->mainLayoutFetched().connect([this](const MainLayoutWidget& layout) {
        CHECK_UI_THREAD();
        if (layout)
        {
            Log::info("[XiboApp] Main window switching from splash to layout");
            mainWindow_->setMainLayout(layout);
            layout->showAll();
        }
        else
        {
            Log::error("[XiboApp] Main window showing splash because no layout was fetched");
            mainWindow_->showSplashScreen();
        }
    });

    manager->overlaysFetched().connect([this](const OverlaysWidgets& overlays) {
        CHECK_UI_THREAD();
        mainWindow_->setOverlays(overlays);
        for (auto&& layout : overlays)
        {
            layout->showAll();
        }
    });

    return manager;
}

GeneralInfo XiboApp::collectGeneralInfo()
{
    GeneralInfo info;

    info.currentDateTime = DateTime::now();
    info.cmsAddress = cmsSettings_.address();
    info.resourcesPath = cmsSettings_.resourcesPath();
    info.codeVersion = AppConfig::codeVersion();
    info.projectVersion = AppConfig::releaseVersion();
    info.screenShotInterval = playerSettings_.screenshotInterval();
    info.displayName = playerSettings_.displayName();
    info.windowWidth = mainWindow_->width();
    info.windowHeight = mainWindow_->height();

    return info;
}

void XiboApp::checkResourceDirectory()
{
    for (auto&& file : fileCache_->cachedFiles())
    {
        if (!fileCache_->valid(file)) continue;

        auto fullPath = cmsSettings_.resourcesPath() / file;
        if (!FileSystem::exists(fullPath))
        {
            Log::error("[XiboApp] Cached file '{}' marked invalid: missing on disk ({})", file, fullPath.string());
            fileCache_->markAsInvalid(file);
            continue;
        }

        auto onDiskHash = Md5Hash::fromFile(fullPath);
        if (!fileCache_->cached(file, onDiskHash))
        {
            Log::error("[XiboApp] Cached file '{}' marked invalid: on-disk hash='{}' differs from cache",
                       file,
                       static_cast<std::string>(onDiskHash));
            fileCache_->markAsInvalid(file);
        }
    }
}

std::unique_ptr<CollectionInterval> XiboApp::createCollectionInterval(XmdsRequestSender& xmdsManager)
{
    auto interval = std::make_unique<CollectionInterval>(
        xmdsManager, *statsRecorder_, *fileCache_, cmsSettings_.resourcesPath(), playerSettings_.displayName());

    interval->updateInterval(playerSettings_.collectInterval());
    playerSettings_.collectInterval().valueChanged().connect(
        std::bind(&CollectionInterval::updateInterval, interval.get(), ph::_1));

    interval->collectionFinished().connect(std::bind(&XiboApp::onCollectionFinished, this, ph::_1));
    interval->scheduleAvailable().connect([this](LayoutSchedule schedule) {
        CHECK_UI_THREAD();
        Log::info("[XiboApp] Collection provided schedule: regularLayouts={}, overlayLayouts={}",
                  schedule.regularLayouts.size(),
                  schedule.overlayLayouts.size());
        scheduler_->reloadSchedule(std::move(schedule));
    });
    interval->filesDownloaded().connect([this]() {
        CHECK_UI_THREAD();
        auto invalidFiles = fileCache_->invalidFiles();
        Log::info("[XiboApp] Files downloaded signal received: invalidFilesRemaining={}", invalidFiles.size());
        for (auto&& file : invalidFiles)
        {
            Log::error("[XiboApp] File still invalid after download cycle: {}", file);
        }
        scheduler_->reloadQueue();
    });
    interval->settingsUpdated().connect([this](const PlayerSettings& settings) {
        CHECK_UI_THREAD();
        Log::info("[XiboApp] Collection updated player settings from CMS");
        playerSettings_.fromFields(settings);
        playerSettings_.saveTo(AppConfig::playerSettingsPath());
    });

    return interval;
}

std::unique_ptr<ScreenShotInterval> XiboApp::createScreenshotInterval(XmdsRequestSender& xmdsManager,
                                                                      Xibo::Window& window)
{
    auto interval = std::make_unique<ScreenShotInterval>(xmdsManager, window);

    interval->updateInterval(playerSettings_.screenshotInterval());
    playerSettings_.screenshotInterval().valueChanged().connect(
        std::bind(&ScreenShotInterval::updateInterval, interval.get(), ph::_1));

    return interval;
}

void XiboApp::onCollectionFinished(const PlayerError& error)
{
    CHECK_UI_THREAD();
    if (error)
    {
        Log::error("[CollectionInterval] {}", error);
    }
    else
    {
        Log::info("[CollectionInterval] Cycle finished successfully");
    }
}
