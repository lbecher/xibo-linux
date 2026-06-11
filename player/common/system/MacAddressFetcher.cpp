#include "MacAddressFetcher.hpp"

#include "common/logger/Logging.hpp"

#include <boost/optional.hpp>

#include <cstdio>
#include <ifaddrs.h>
#include <linux/if_packet.h>
#include <net/if.h>

namespace
{
const MacAddress UndefinedMacAddress{"00:00:00:00:00:00"};
const std::size_t MacAddressLength = 6;

bool isLinkAddress(const ifaddrs& interface)
{
    return interface.ifa_addr != nullptr && interface.ifa_addr->sa_family == AF_PACKET;
}

bool isLoopback(const ifaddrs& interface)
{
    return (interface.ifa_flags & IFF_LOOPBACK) != 0;
}

std::string formatMacAddress(const unsigned char* mac)
{
    char buffer[18] = {0};
    std::snprintf(buffer,
                  sizeof(buffer),
                  "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
                  mac[0],
                  mac[1],
                  mac[2],
                  mac[3],
                  mac[4],
                  mac[5]);
    return buffer;
}

bool isUndefined(const std::string& macAddress)
{
    return macAddress == static_cast<std::string>(UndefinedMacAddress);
}

bool canUseForMacAddress(const ifaddrs& interface)
{
    if (!isLinkAddress(interface) || isLoopback(interface))
    {
        return false;
    }

    auto* linkAddress = reinterpret_cast<sockaddr_ll*>(interface.ifa_addr);
    if (linkAddress->sll_halen < MacAddressLength)
    {
        return false;
    }

    return !isUndefined(formatMacAddress(linkAddress->sll_addr));
}

boost::optional<MacAddress> macAddressFrom(const ifaddrs& interface)
{
    if (!canUseForMacAddress(interface))
    {
        return boost::none;
    }

    auto* linkAddress = reinterpret_cast<sockaddr_ll*>(interface.ifa_addr);
    return MacAddress{formatMacAddress(linkAddress->sll_addr)};
}
}

MacAddress MacAddressFetcher::fetch()
{
    return fetch(std::string{});
}

MacAddress MacAddressFetcher::fetch(const std::string& interfaceName)
{
    ifaddrs* interfaces = nullptr;
    if (getifaddrs(&interfaces) != 0)
    {
        Log::info("Failed to get MAC address: unable to list interfaces");
        return UndefinedMacAddress;
    }

    boost::optional<MacAddress> firstAvailableMacAddress;
    for (auto* it = interfaces; it != nullptr; it = it->ifa_next)
    {
        if (it->ifa_name == nullptr)
        {
            continue;
        }

        auto macAddress = macAddressFrom(*it);
        if (!macAddress)
        {
            continue;
        }

        if (!firstAvailableMacAddress)
        {
            firstAvailableMacAddress = macAddress;
        }

        if (!interfaceName.empty() && interfaceName == it->ifa_name)
        {
            freeifaddrs(interfaces);
            return *macAddress;
        }
    }

    freeifaddrs(interfaces);

    if (interfaceName.empty() && firstAvailableMacAddress)
    {
        return *firstAvailableMacAddress;
    }

    Log::info("Failed to get MAC address: selected interface '{}' was not found or has no hardware address",
              interfaceName);
    return UndefinedMacAddress;
}
