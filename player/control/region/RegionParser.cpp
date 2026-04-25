#include "RegionParser.hpp"

#include "control/cache/UnsafeFaultCodes.hpp"
#include "control/cache/UnsafeItemStore.hpp"
#include "control/region/RegionImpl.hpp"
#include "control/region/RegionResources.hpp"

#include "control/media/MediaParsersRepo.hpp"

#include "common/logger/Logging.hpp"

const int DefaultRegionZorder = 0;
const bool DefaultRegionLoop = false;

using namespace std::string_literals;

RegionParser::RegionParser(bool globalStatEnabled, int layoutId) :
    globalStatEnabled_{globalStatEnabled}, layoutId_{layoutId}
{
}

std::unique_ptr<Xibo::Region> RegionParser::regionFrom(const XmlNode& node)
{
    try
    {
        auto options = optionsFrom(node);
        auto region = std::make_unique<RegionImpl>(options);

        addMedia(*region, node);

        if (region->mediaList().empty())
        {
            UnsafeItemStore::instance().addUnsafeItem(UnsafeItemType::Region,
                                                      static_cast<int>(UnsafeFaultCode::XlfNoContent),
                                                      options.layoutId,
                                                      std::to_string(options.id),
                                                      "There are no valid widgets inside one of the regions in this layout",
                                                      60);
            throw Error{"RegionParser", "Unable to parse any valid media from region"};
        }

        return region;
    }
    catch (PlayerRuntimeError& e)
    {
        throw Error{"RegionParser - " + e.domain(), e.message()};
    }
    catch (std::exception& e)
    {
        throw Error{"RegionParser", e.what()};
    }
}

RegionPosition RegionParser::positionFrom(const XmlNode& node)
{
    RegionPosition position;

    position.left = static_cast<int>(node.get<float>(XlfResources::Region::Left));
    position.top = static_cast<int>(node.get<float>(XlfResources::Region::Top));
    position.zorder = node.get<int>(XlfResources::Region::Zindex, DefaultRegionZorder);
    if (position.zorder < 0)
    {
        position.zorder = DefaultRegionZorder;
    }

    return position;
}

RegionOptions RegionParser::optionsFrom(const XmlNode& node)
{
    RegionOptions options;

    options.layoutId = layoutId_;
    options.id = node.get<int>(XlfResources::Region::Id);
    options.width = static_cast<int>(node.get<float>(XlfResources::Region::Width));
    options.height = static_cast<int>(node.get<float>(XlfResources::Region::Height));
    options.loop = static_cast<RegionOptions::Loop>(node.get<bool>(XlfResources::Region::Loop, DefaultRegionLoop));

    return options;
}

void RegionParser::addMedia(Xibo::Region& region, const XmlNode& regionNode)
{
    auto options = optionsFrom(regionNode);

    for (auto [nodeName, node] : regionNode)
    {
        if (nodeName != XlfResources::MediaNode) continue;

        auto mediaType = mediaTypeFrom(node);
        Log::info("[RegionParser] Media node id={} type={} render={}",
                  node.get<int>(XlfResources::Media::Id, -1),
                  mediaType.type,
                  mediaType.render);

        auto parser = MediaParsersRepo::get(mediaType);
        if (parser)
        {
            // TODO: don't use width/height if media type is widget-less
            int width = region.view()->width();
            int height = region.view()->height();

            parser->context(options.layoutId, options.id);
            try
            {
                region.addMedia(parser->mediaFrom(node, width, height, globalStatEnabled_));
            }
            catch (const MediaParser::Error& e)
            {
                Log::error("[RegionParser] {}", e.what());
            }
        }
    }
}

MediaOptions::Type RegionParser::mediaTypeFrom(const XmlNode& node)
{
    auto type = node.get<std::string>(XlfResources::Media::Type);
    auto render = node.get<std::string>(XlfResources::Media::Render);

    return {type, render};
}
