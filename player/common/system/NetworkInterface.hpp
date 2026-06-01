#pragma once

#include <string>
#include <vector>

struct NetworkInterfaceStatus
{
    bool exists = false;
    bool up = false;
    bool hasIpAddress = false;
};

namespace NetworkInterface
{
    std::vector<std::string> availableNames();
    NetworkInterfaceStatus status(const std::string& name);
    bool waitForIpAddress(const std::string& name, int timeoutSeconds);
}
