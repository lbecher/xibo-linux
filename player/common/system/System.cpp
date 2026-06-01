#include "System.hpp"

#include "common/system/Dbus.hpp"
#include "common/system/HardwareKeyGenerator.hpp"
#include "common/system/MacAddressFetcher.hpp"

#include <limits.h>
#include <signal.h>
#include <array>
#include <boost/optional.hpp>

namespace
{
std::string g_networkInterface;
boost::optional<MacAddress> g_macAddress;
boost::optional<HardwareKey> g_hardwareKey;
}

MacAddress System::macAddress()
{
    if (!g_macAddress)
    {
        g_macAddress = MacAddressFetcher::fetch(g_networkInterface);
    }

    return *g_macAddress;
}

HardwareKey System::hardwareKey()
{
    if (!g_hardwareKey)
    {
        g_hardwareKey = HardwareKeyGenerator::generate();
    }

    return *g_hardwareKey;
}

void System::networkInterface(const std::string& interfaceName)
{
    if (g_networkInterface == interfaceName)
    {
        return;
    }

    g_networkInterface = interfaceName;
    g_macAddress = boost::none;
    g_hardwareKey = boost::none;
}

std::string System::networkInterface()
{
    return g_networkInterface;
}

void System::preventSleep()
{
    Dbus dbus;
    dbus.preventSleep();
}

void System::terminateProccess(int processId)
{
    kill(processId, SIGTERM);
}

int System::parentProcessId()
{
    return getppid();
}

Hostname System::hostname()
{
    std::array<char, HOST_NAME_MAX + 1> buffer{};
    gethostname(buffer.data(), HOST_NAME_MAX);
    return Hostname{buffer.data()};
}
