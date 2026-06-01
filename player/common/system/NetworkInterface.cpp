#include "NetworkInterface.hpp"

#include "common/logger/Logging.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <thread>

namespace
{
bool isIpAddress(int family)
{
    return family == AF_INET || family == AF_INET6;
}
}

std::vector<std::string> NetworkInterface::availableNames()
{
    std::vector<std::string> names;
    ifaddrs* interfaces = nullptr;
    if (getifaddrs(&interfaces) != 0)
    {
        Log::error("[NetworkInterface] Unable to list interfaces: {}", std::strerror(errno));
        return names;
    }

    for (auto* it = interfaces; it != nullptr; it = it->ifa_next)
    {
        if (it->ifa_name == nullptr || (it->ifa_flags & IFF_LOOPBACK))
        {
            continue;
        }

        std::string name{it->ifa_name};
        if (std::find(names.begin(), names.end(), name) == names.end())
        {
            names.push_back(name);
        }
    }

    freeifaddrs(interfaces);
    std::sort(names.begin(), names.end());
    return names;
}

NetworkInterfaceStatus NetworkInterface::status(const std::string& name)
{
    NetworkInterfaceStatus result;
    if (name.empty())
    {
        return result;
    }

    ifaddrs* interfaces = nullptr;
    if (getifaddrs(&interfaces) != 0)
    {
        Log::error("[NetworkInterface] Unable to inspect interface '{}': {}", name, std::strerror(errno));
        return result;
    }

    for (auto* it = interfaces; it != nullptr; it = it->ifa_next)
    {
        if (it->ifa_name == nullptr || name != it->ifa_name)
        {
            continue;
        }

        result.exists = true;
        result.up = (it->ifa_flags & IFF_UP) != 0;
        if (it->ifa_addr != nullptr && isIpAddress(it->ifa_addr->sa_family))
        {
            result.hasIpAddress = true;
        }
    }

    freeifaddrs(interfaces);
    return result;
}

bool NetworkInterface::waitForIpAddress(const std::string& name, int timeoutSeconds)
{
    for (int elapsed = 0; elapsed <= timeoutSeconds; ++elapsed)
    {
        auto current = status(name);
        if (!current.exists || !current.up)
        {
            return false;
        }

        if (current.hasIpAddress)
        {
            return true;
        }

        if (elapsed < timeoutSeconds)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    return false;
}
