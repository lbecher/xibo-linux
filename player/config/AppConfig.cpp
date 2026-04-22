#include "AppConfig.hpp"

#include <cstring>

#include "common/PlayerRuntimeError.hpp"
#include "common/fs/FileSystem.hpp"
#include "common/logger/Logging.hpp"
#include "GitHash.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <linux/limits.h>
#include <unistd.h>
#include <vector>

namespace
{
FilePath ensureDirectoryExists(const FilePath& path)
{
    if (!FileSystem::exists(path))
    {
        if (!std::filesystem::create_directories(path.string()))
        {
            throw PlayerRuntimeError{"AppConfig", fmt::format("Unable to create directory {}", path)};
        }
    }

    return path;
}

FilePath userConfigDirectory()
{
    if (const char* xdgConfigHome = std::getenv("XDG_CONFIG_HOME"); xdgConfigHome && *xdgConfigHome)
    {
        return FilePath{xdgConfigHome} / "xibo-player";
    }

    if (const char* home = std::getenv("HOME"); home && *home)
    {
        return FilePath{home} / ".config" / "xibo-player";
    }

    throw PlayerRuntimeError{"AppConfig", "HOME or XDG_CONFIG_HOME is not set"};
}

bool hasUiFile(const FilePath& directory)
{
    return FileSystem::exists(directory / "ui.glade");
}

FilePath resolveResourcesDirectory(const FilePath& binaryDir)
{
    std::vector<FilePath> candidates;

    const char* resourcePath = std::getenv("XIBO_RESOURCE_PATH");
    if (resourcePath && std::strlen(resourcePath) > 0)
    {
        candidates.emplace_back(resourcePath);
    }

    candidates.push_back(binaryDir);
    candidates.push_back(binaryDir / ".." / ".." / "player" / "resources");
    candidates.push_back(binaryDir / ".." / "share" / "xibo-player");
    candidates.push_back(binaryDir / ".." / ".." / "share" / "xibo-player");

    for (const auto& candidate : candidates)
    {
        if (hasUiFile(candidate))
        {
            return candidate;
        }
    }

    return binaryDir;
}
} // namespace

FilePath AppConfig::resourceDirectory_;

std::string AppConfig::version()
{
    return releaseVersion() + "-" + codeVersion();
}

std::string AppConfig::releaseVersion()
{
    // Update this with each release.
    return std::string{"1.8 R"} + codeVersion() + GIT_HASH;
}

std::string AppConfig::codeVersion()
{
    // Update this with each release
    return "7";
}

FilePath AppConfig::resourceDirectory()
{
    return resourceDirectory_;
}

void AppConfig::resourceDirectory(const FilePath& directory)
{
    if (!FileSystem::exists(directory))
        throw PlayerRuntimeError{
            "AppConfig", "Resource directory doesn't exist. Create or use exsiting one in the player options app."};

    resourceDirectory_ = directory;
}

FilePath AppConfig::configDirectory()
{
    return ensureDirectoryExists(userConfigDirectory());
}

FilePath AppConfig::oldConfigDirectory()
{
    return execDirectory();
}

FilePath AppConfig::publicKeyPath()
{
    return configDirectory() / "id_rsa.pub";
}

FilePath AppConfig::privateKeyPath()
{
    return configDirectory() / "id_rsa";
}

FilePath AppConfig::cmsSettingsPath()
{
    return configDirectory() / "cmsSettings.xml";
}

FilePath AppConfig::playerSettingsPath()
{
    return configDirectory() / "playerSettings.xml";
}

FilePath AppConfig::schedulePath()
{
    return configDirectory() / "schedule.xml";
}

FilePath AppConfig::cachePath()
{
    return configDirectory() / "cacheFile.xml";
}

FilePath AppConfig::statsCache()
{
    return configDirectory() / "stats.sqlite";
}

FilePath AppConfig::additionalResourcesDirectory()
{
    return resolveResourcesDirectory(execDirectory());
}

FilePath AppConfig::splashScreenPath()
{
    return additionalResourcesDirectory() / "splash.jpg";
}

FilePath AppConfig::uiFile()
{
    return additionalResourcesDirectory() / "ui.glade";
}

FilePath AppConfig::execDirectory()
{
    // Resolve the directory that contains the running executable.
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    assert(count != -1);
    return FilePath{std::filesystem::path(std::string(result, static_cast<size_t>(count))).parent_path()};
}

std::string AppConfig::playerBinary()
{
    return (execDirectory() / "xibo-player").string();
}

std::string AppConfig::optionsBinary()
{
    return (execDirectory() / "xibo-options").string();
}
