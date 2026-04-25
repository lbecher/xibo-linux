#include "VideoFactory.hpp"

#include "common/constants.hpp"
#include "common/logger/Logging.hpp"
#include "control/media/player/GstMediaPlayer.hpp"
#include "control/media/player/PlayableMedia.hpp"

std::unique_ptr<Xibo::Media> VideoFactory::create(const MediaPlayerOptions& options, int width, int height)
{
    auto videoPlayer = createPlayer(options, width, height);
    return std::make_unique<PlayableMedia>(options, std::move(videoPlayer));
}

std::unique_ptr<Xibo::MediaPlayer> VideoFactory::createPlayer(const MediaPlayerOptions& options, int width, int height)
{
    Log::info("[VideoFactory] Creating native video player for mediaId={} uri={}", options.id, options.uri.string());
    auto player = std::make_unique<GstMediaPlayer>(options);

    player->setVolume(options.muted == MediaPlayerOptions::Mute::Enable ? MinVolume : MaxVolume);
    player->setAspectRatio(options.geometry.scaleType);
    player->load(options.uri);

    // TODO set window explicitly if no internal window created
    if (auto output = player->outputWindow())
    {
        output->setSize(width, height);
    }

    return player;
}
