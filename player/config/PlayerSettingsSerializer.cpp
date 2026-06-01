#include "PlayerSettingsSerializer.hpp"

void PlayerSettingsSerializer::loadSettingsFrom(const FilePath& file, PlayerSettings& settings)
{
    loadFromImpl(loadXmlFrom(file),
                 settings.size_,
                 settings.position_,
                 settings.logLevel_,
                 settings.displayName_,
                 settings.preventSleep_,
                 settings.statsEnabled_,
                 settings.collectInterval_,
                 settings.xmrNetworkAddress_,
                 settings.xmrType_,
                 settings.xmrWebSocketAddress_,
                 settings.xmrCmsKey_,
                 settings.enableShellCommands_,
                 settings.embeddedServerPort_,
                 settings.screenshotInterval_,
                 settings.networkInterface_);
}

void PlayerSettingsSerializer::saveSettingsTo(const FilePath& file, const PlayerSettings& settings)
{
    auto tree = saveToImpl(settings.size_,
                           settings.position_,
                           settings.logLevel_,
                           settings.displayName_,
                           settings.preventSleep_,
                           settings.statsEnabled_,
                           settings.collectInterval_,
                           settings.xmrNetworkAddress_,
                           settings.xmrType_,
                           settings.xmrWebSocketAddress_,
                           settings.xmrCmsKey_,
                           settings.enableShellCommands_,
                           settings.embeddedServerPort_,
                           settings.screenshotInterval_,
                           settings.networkInterface_);
    saveXmlTo(file, tree);
}

XmlDocVersion PlayerSettingsSerializer::currentVersion() const
{
    return XmlDocVersion{"2"};
}
