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

GstMediaPlayer::GstMediaPlayer(const MediaPlayerOptions& options) :
    playbin_(gst_element_factory_make("playbin", "playbin")),
    videoSink_(gst_element_factory_make("gtk4paintablesink", "gtk4paintablesink")),
    glSinkBin_(nullptr),
    options_(options)
{
    if (!playbin_) throw Error{"GstMediaPlayer", "Unable to create player: playbin is missing."};
    if (!videoSink_)
    {
        throw Error{"GstMediaPlayer",
                    "Unable to create player: gtk4paintablesink is missing. Install the GStreamer GTK4 sink plugin."};
    }

    g_object_set(playbin_, "video-sink", videoSink_, nullptr);

    GdkPaintable* paintable = nullptr;
    g_object_get(videoSink_, "paintable", &paintable, nullptr);
    if (paintable)
    {
        auto picture = std::make_unique<Gtk::Picture>(Glib::wrap(paintable, false));
        picture->set_can_shrink(true);
        picture->set_hexpand(true);
        picture->set_vexpand(true);
        outputWindow_ = std::make_shared<OutputWindowGtk>(std::move(picture));
    }
    else
    {
        throw Error{"GstMediaPlayer", "Unable to create player output: gtk4paintablesink did not provide a paintable"};
    }

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
        default: break;
    }
    return true;
}

SignalPlaybackFinished& GstMediaPlayer::playbackFinished()
{
    return playbackFinished_;
}
