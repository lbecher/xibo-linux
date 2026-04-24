#include "PlayerSettings.hpp"

#include "common/logger/Logging.hpp"
#include "config/PlayerSettingsSerializer.hpp"

void PlayerSettings::fromFields(const PlayerSettings& settings)
{
    auto setIfChanged = [](auto& target, const auto& incomingValue) {
        if (target.value() != incomingValue)
        {
            target.setValue(incomingValue);
        }
    };

    auto setTupleIfChanged = [](auto& target, const auto& incomingValues) {
        if (target.values() != incomingValues)
        {
            target.setValue(incomingValues);
        }
    };

    setIfChanged(collectInterval_, settings.collectInterval().value());
    setIfChanged(statsEnabled_, settings.statsEnabled().value());
    setIfChanged(xmrNetworkAddress_, settings.xmrNetworkAddress().value());
    setIfChanged(xmrType_, settings.xmrType().value());
    setIfChanged(xmrWebSocketAddress_, settings.xmrWebSocketAddress().value());
    setIfChanged(xmrCmsKey_, settings.xmrCmsKey().value());
    setTupleIfChanged(size_, settings.size().values());
    setTupleIfChanged(position_, settings.position().values());
    setIfChanged(logLevel_, settings.logLevel().value());
    setIfChanged(screenshotInterval_, settings.screenshotInterval().value());
    setIfChanged(enableShellCommands_, settings.enableShellCommands().value());
    setIfChanged(embeddedServerPort_, settings.embeddedServerPort().value());
    setIfChanged(preventSleep_, settings.preventSleep().value());
    setIfChanged(displayName_, settings.displayName().value());
}

void PlayerSettings::fromFile(const FilePath& file)
{
    try
    {
        PlayerSettingsSerializer serializer;
        serializer.loadSettingsFrom(file, *this);
    }
    catch (PlayerRuntimeError& e)
    {
        Log::error("[PlayerSettings] Load error: {}", e.message());
    }
    catch (std::exception& e)
    {
        Log::error("[PlayerSettings] Load error: {}", e.what());
    }
}

void PlayerSettings::saveTo(const FilePath& file)
{
    try
    {
        PlayerSettingsSerializer serializer;
        serializer.saveSettingsTo(file, *this);
    }
    catch (PlayerRuntimeError& e)
    {
        Log::error("[PlayerSettings] Save error: {}", e.message());
    }
    catch (std::exception& e)
    {
        Log::error("[PlayerSettings] Save error: {}", e.what());
    }
}

Field<int>& PlayerSettings::collectInterval()
{
    return collectInterval_;
}

const Field<int>& PlayerSettings::collectInterval() const
{
    return collectInterval_;
}

Field<bool>& PlayerSettings::statsEnabled()
{
    return statsEnabled_;
}

const Field<bool>& PlayerSettings::statsEnabled() const
{
    return statsEnabled_;
}

Field<std::string>& PlayerSettings::xmrNetworkAddress()
{
    return xmrNetworkAddress_;
}

const Field<std::string>& PlayerSettings::xmrNetworkAddress() const
{
    return xmrNetworkAddress_;
}

Field<std::string>& PlayerSettings::xmrType()
{
    return xmrType_;
}

const Field<std::string>& PlayerSettings::xmrType() const
{
    return xmrType_;
}

Field<std::string>& PlayerSettings::xmrWebSocketAddress()
{
    return xmrWebSocketAddress_;
}

const Field<std::string>& PlayerSettings::xmrWebSocketAddress() const
{
    return xmrWebSocketAddress_;
}

Field<std::string>& PlayerSettings::xmrCmsKey()
{
    return xmrCmsKey_;
}

const Field<std::string>& PlayerSettings::xmrCmsKey() const
{
    return xmrCmsKey_;
}

Field<std::string>& PlayerSettings::logLevel()
{
    return logLevel_;
}

const Field<std::string>& PlayerSettings::logLevel() const
{
    return logLevel_;
}

Field<int>& PlayerSettings::screenshotInterval()
{
    return screenshotInterval_;
}

const Field<int>& PlayerSettings::screenshotInterval() const
{
    return screenshotInterval_;
}

Field<bool>& PlayerSettings::enableShellCommands()
{
    return enableShellCommands_;
}

const Field<bool>& PlayerSettings::enableShellCommands() const
{
    return enableShellCommands_;
}

Field<unsigned short>& PlayerSettings::embeddedServerPort()
{
    return embeddedServerPort_;
}

const Field<unsigned short>& PlayerSettings::embeddedServerPort() const
{
    return embeddedServerPort_;
}

Field<bool>& PlayerSettings::preventSleep()
{
    return preventSleep_;
}

const Field<bool>& PlayerSettings::preventSleep() const
{
    return preventSleep_;
}

Field<std::string>& PlayerSettings::displayName()
{
    return displayName_;
}

const Field<std::string>& PlayerSettings::displayName() const
{
    return displayName_;
}

PlayerSettings::SizeField& PlayerSettings::size()
{
    return size_;
}

const PlayerSettings::SizeField& PlayerSettings::size() const
{
    return size_;
}

PlayerSettings::PositionField& PlayerSettings::position()
{
    return position_;
}

const PlayerSettings::PositionField& PlayerSettings::position() const
{
    return position_;
}
