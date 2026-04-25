#include "GstMediaPlayer.hpp"

#include "control/cache/UnsafeFaultCodes.hpp"
#include "control/cache/UnsafeItemStore.hpp"
#include "common/logger/Logging.hpp"
#include "common/types/Uri.hpp"
#include "common/constants.hpp"
#include "control/widgets/gtk/OutputWindowGtk.hpp"

#include <gdkmm/paintable.h>
#include <gst/gst.h>
#include <gtkmm/picture.h>
#include <algorithm>

GstMediaPlayer::GstMediaPlayer(const MediaPlayerOptions& options) :
    playbin_(gst_element_factory_make("playbin", "playbin")),
    videoSink_(nullptr),
    glSinkBin_(nullptr),
    options_(options),
    pipelineLogged_(false)
{
    if (!playbin_) throw Error{"GstMediaPlayer", "Unable to create player: playbin is missing."};
    logRuntimeCapabilities();

    auto* sink = createVideoSink();
    if (!sink)
    {
        throw Error{"GstMediaPlayer",
                    "Unable to create player: gtk4paintablesink is missing. Install the GStreamer GTK4 sink plugin."};
    }

    g_object_set(playbin_, "video-sink", sink, nullptr);
    createOutputWindow();

    auto bus = gst_element_get_bus(playbin_);
    busWatchId_ = gst_bus_add_watch(bus, static_cast<GstBusFunc>(&GstMediaPlayer::busMessageWatch), this);
    gst_object_unref(bus);
}

GstMediaPlayer::~GstMediaPlayer()
{
    gst_element_set_state(playbin_, GST_STATE_NULL);
    gst_object_unref(playbin_);  // videoSink_ should be unrefed as a child
    g_source_remove(busWatchId_);
    // check gst_bus_remove_watch
}

void GstMediaPlayer::load(const Uri& uri)
{
    g_object_set(playbin_, "uri", uri.string().c_str(), nullptr);
}

void GstMediaPlayer::setVolume(int volume)
{
    check(volume);
    g_object_set(playbin_, "volume", volume / static_cast<double>(MaxVolume), nullptr);
}

void GstMediaPlayer::setAspectRatio(MediaGeometry::ScaleType scaleType)
{
    bool aspectRatio = scaleType == MediaGeometry::ScaleType::Scaled ? true : false;
    g_object_set(playbin_, "force-aspect-ratio", aspectRatio, nullptr);
}

void GstMediaPlayer::check(int volume)
{
    if (volume < MinVolume || volume > MaxVolume) throw Error{"GstMediaPlayer", "Volume should be in [0-100] range"};
}

void GstMediaPlayer::play()
{
    pipelineLogged_ = false;
    gst_element_set_state(playbin_, GST_STATE_PLAYING);
}

void GstMediaPlayer::stop()
{
    gst_element_set_state(playbin_, GST_STATE_NULL);
}

void GstMediaPlayer::showOutputWindow()
{
    if (outputWindow_)
    {
        outputWindow_->show();
    }
}

void GstMediaPlayer::hideOutputWindow()
{
    if (outputWindow_)
    {
        outputWindow_->hide();
    }
}

void GstMediaPlayer::setOutputWindow(const std::shared_ptr<Xibo::OutputWindow>& outputWindow)
{
    assert(outputWindow);

    outputWindow_ = outputWindow;
}

const std::shared_ptr<Xibo::OutputWindow>& GstMediaPlayer::outputWindow() const
{
    return outputWindow_;
}

// we don't need to unref bus here
gboolean GstMediaPlayer::busMessageWatch(GstBus* /*bus*/, GstMessage* msg, gpointer data)
{
    switch (msg->type)
    {
        case GST_MESSAGE_EOS:
        {
            assert(data);
            auto player = reinterpret_cast<GstMediaPlayer*>(data);

            Log::debug("[GstMediaPlayer] End of stream");
            gst_element_set_state(player->playbin_, GST_STATE_NULL);
            player->playbackFinished_();
            break;
        }
        case GST_MESSAGE_ERROR:
        {
            assert(data);
            auto player = reinterpret_cast<GstMediaPlayer*>(data);
            GError* err = nullptr;
            gchar* debug_info = nullptr;

            gst_message_parse_error(msg, &err, &debug_info);
            Log::error("[GstMediaPlayer] Error from element {}: {}", msg->src->name, err->message);
            player->stop();
            UnsafeItemStore::instance().addUnsafeItem(UnsafeItemType::Media,
                                                      static_cast<int>(UnsafeFaultCode::VideoUnexpected),
                                                      player->options_.layoutId,
                                                      std::to_string(player->options_.id),
                                                      std::string{"Video Failed: "} +
                                                          (err && err->message ? err->message : "unknown error"),
                                                      120);
            player->playbackFinished_();
            if (debug_info)
            {
                Log::debug("[GstMediaPlayer] Debug details: {}", debug_info);
            }
            g_clear_error(&err);
            g_free(debug_info);
            return false;
        }
        case GST_MESSAGE_ASYNC_DONE:
        {
            assert(data);
            auto player = reinterpret_cast<GstMediaPlayer*>(data);
            player->logSelectedPipeline();
            break;
        }
        case GST_MESSAGE_STATE_CHANGED:
        {
            assert(data);
            auto player = reinterpret_cast<GstMediaPlayer*>(data);
            if (GST_MESSAGE_SRC(msg) == GST_OBJECT(player->playbin_))
            {
                GstState oldState = GST_STATE_NULL;
                GstState newState = GST_STATE_NULL;
                GstState pendingState = GST_STATE_NULL;
                gst_message_parse_state_changed(msg, &oldState, &newState, &pendingState);
                if ((newState == GST_STATE_PAUSED || newState == GST_STATE_PLAYING) &&
                    pendingState == GST_STATE_VOID_PENDING)
                {
                    player->logSelectedPipeline();
                }
            }
            break;
        }
        default: break;
    }
    return true;
}

SignalPlaybackFinished& GstMediaPlayer::playbackFinished()
{
    return playbackFinished_;
}

GstElement* GstMediaPlayer::createVideoSink()
{
    if (auto* sinkBin = tryCreateGlSinkBin())
    {
        Log::info("[GstMediaPlayer] Using GL video sink pipeline");
        return sinkBin;
    }

    auto* sink = tryCreateGtkPaintableSink();
    if (sink)
    {
        Log::info("[GstMediaPlayer] Using GTK4 paintable sink fallback");
    }
    return sink;
}

GstElement* GstMediaPlayer::tryCreateGtkPaintableSink()
{
    videoSink_ = gst_element_factory_make("gtk4paintablesink", "gtk4paintablesink");
    return videoSink_;
}

GstElement* GstMediaPlayer::tryCreateGlSinkBin()
{
    auto* glUpload = gst_element_factory_make("glupload", "glupload");
    auto* glColorConvert = gst_element_factory_make("glcolorconvert", "glcolorconvert");
    auto* gtkPaintableSink = gst_element_factory_make("gtk4paintablesink", "gtk4paintablesink");
    if (!glUpload || !glColorConvert || !gtkPaintableSink)
    {
        if (glUpload) gst_object_unref(glUpload);
        if (glColorConvert) gst_object_unref(glColorConvert);
        if (gtkPaintableSink) gst_object_unref(gtkPaintableSink);
        return nullptr;
    }

    auto* sinkBin = gst_bin_new("gtk4glsinkbin");
    if (!sinkBin)
    {
        gst_object_unref(glUpload);
        gst_object_unref(glColorConvert);
        gst_object_unref(gtkPaintableSink);
        return nullptr;
    }

    gst_bin_add_many(GST_BIN(sinkBin), glUpload, glColorConvert, gtkPaintableSink, nullptr);
    if (!gst_element_link_many(glUpload, glColorConvert, gtkPaintableSink, nullptr))
    {
        gst_object_unref(sinkBin);
        return nullptr;
    }

    auto* sinkPad = gst_element_get_static_pad(glUpload, "sink");
    if (!sinkPad)
    {
        gst_object_unref(sinkBin);
        return nullptr;
    }

    auto* ghostPad = gst_ghost_pad_new("sink", sinkPad);
    gst_object_unref(sinkPad);
    if (!ghostPad)
    {
        gst_object_unref(sinkBin);
        return nullptr;
    }

    auto padAdded = gst_element_add_pad(sinkBin, ghostPad);
    if (!padAdded)
    {
        gst_object_unref(ghostPad);
        gst_object_unref(sinkBin);
        return nullptr;
    }

    videoSink_ = gtkPaintableSink;
    glSinkBin_ = sinkBin;
    return glSinkBin_;
}

void GstMediaPlayer::createOutputWindow()
{
    GdkPaintable* paintable = nullptr;
    g_object_get(videoSink_, "paintable", &paintable, nullptr);
    if (!paintable)
    {
        throw Error{"GstMediaPlayer", "Unable to create player output: gtk4paintablesink did not provide a paintable"};
    }

    auto picture = std::make_unique<Gtk::Picture>(Glib::wrap(paintable, false));
    picture->set_can_shrink(true);
    picture->set_hexpand(true);
    picture->set_vexpand(true);
    outputWindow_ = std::make_shared<OutputWindowGtk>(std::move(picture));
}

void GstMediaPlayer::logRuntimeCapabilities() const
{
    Log::info("[GstMediaPlayer] Runtime capabilities: glupload={}, glcolorconvert={}, gtk4paintablesink={}, vaapih264dec={}, vaapih265dec={}, v4l2h264dec={}, v4l2slh264dec={}",
              hasElementFactory("glupload"),
              hasElementFactory("glcolorconvert"),
              hasElementFactory("gtk4paintablesink"),
              hasElementFactory("vaapih264dec"),
              hasElementFactory("vaapih265dec"),
              hasElementFactory("v4l2h264dec"),
              hasElementFactory("v4l2slh264dec"));
}

bool GstMediaPlayer::hasElementFactory(const char* factoryName) const
{
    auto* factory = gst_element_factory_find(factoryName);
    if (!factory) return false;

    gst_object_unref(factory);
    return true;
}

void GstMediaPlayer::logSelectedPipeline()
{
    if (pipelineLogged_) return;

    auto factories = collectElementFactories();
    if (factories.empty()) return;

    const std::vector<std::string> decoderFragments{
        "dec", "vaapi", "v4l2", "nv", "avdec", "openh264", "omx", "msdk", "d3d11", "vtdec"};
    const std::vector<std::string> sinkFragments{"sink"};
    const std::vector<std::string> convertFragments{"convert", "upload", "scale"};

    std::vector<std::string> decoders;
    std::vector<std::string> sinks;
    std::vector<std::string> converters;

    for (const auto& factory : factories)
    {
        if (hasNameFragment(factory, decoderFragments)) decoders.push_back(factory);
        if (hasNameFragment(factory, sinkFragments)) sinks.push_back(factory);
        if (hasNameFragment(factory, convertFragments)) converters.push_back(factory);
    }

    auto joinNames = [](const std::vector<std::string>& names) {
        if (names.empty()) return std::string{"none"};

        std::string joined;
        for (size_t i = 0; i < names.size(); ++i)
        {
            if (i != 0) joined += ", ";
            joined += names[i];
        }
        return joined;
    };

    Log::info("[GstMediaPlayer] Active pipeline factories: decoders=[{}] converters=[{}] sinks=[{}]",
              joinNames(decoders),
              joinNames(converters),
              joinNames(sinks));
    pipelineLogged_ = true;
}

std::vector<std::string> GstMediaPlayer::collectElementFactories() const
{
    std::vector<std::string> factories;

    auto* iterator = gst_bin_iterate_recurse(GST_BIN(playbin_));
    if (!iterator) return factories;

    GValue item = G_VALUE_INIT;
    while (gst_iterator_next(iterator, &item) == GST_ITERATOR_OK)
    {
        auto* element = GST_ELEMENT(g_value_get_object(&item));
        if (element)
        {
            auto* factory = gst_element_get_factory(element);
            if (factory)
            {
                factories.emplace_back(gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory)));
            }
        }
        g_value_reset(&item);
    }

    g_value_unset(&item);
    gst_iterator_free(iterator);

    std::sort(factories.begin(), factories.end());
    factories.erase(std::unique(factories.begin(), factories.end()), factories.end());
    return factories;
}

bool GstMediaPlayer::hasNameFragment(const std::string& value, const std::vector<std::string>& fragments)
{
    return std::any_of(fragments.begin(), fragments.end(), [&value](const auto& fragment) {
        return value.find(fragment) != std::string::npos;
    });
}
