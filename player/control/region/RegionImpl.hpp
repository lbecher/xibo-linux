#pragma once

#include "control/media/Media.hpp"
#include "control/region/Region.hpp"
#include "control/region/RegionOptions.hpp"
#include "control/widgets/FixedContainer.hpp"

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

class RegionImpl : public Xibo::Region, private boost::noncopyable
{
    static constexpr const int MediaOrder = 0;
    static constexpr const int FirstMediaIndex = 0;

public:
    RegionImpl(const RegionOptions& options);

    void addMedia(std::unique_ptr<Xibo::Media>&& media) override;
    void start() override;
    void stop() override;
    int id() const override;
    bool hasMediaId(int mediaId) const override;
    bool hasActiveMediaId(int mediaId) const override;
    bool navigateToMediaId(int mediaId) override;
    bool showNextMedia() override;
    bool setActiveMediaDuration(int duration) override;
    bool extendActiveMediaDuration(int duration) override;
    SignalRegionExpired& expired() override;
    std::shared_ptr<Xibo::Widget> view() override;

    const MediaList& mediaList() const override;

private:
    void placeMedia(size_t mediaIndex);
    void removeMedia(size_t mediaIndex);
    void onMediaDurationTimeout();

    std::pair<int, int> calcMediaPosition(Xibo::Media& media);

    bool shouldBeMediaReplaced() const;
    size_t getNextMediaIndex() const;
    bool isExpired() const;
    boost::optional<size_t> mediaIndexById(int mediaId) const;

private:
    RegionOptions options_;
    std::shared_ptr<Xibo::FixedContainer> view_;
    MediaList mediaList_;
    size_t currentMediaIndex_;
    SignalRegionExpired regionExpired_;
};
