#pragma once

#include "common/system/HardwareKey.hpp"
#include "common/system/Hostname.hpp"
#include "common/system/MacAddress.hpp"

#include <string>

namespace System
{
    MacAddress macAddress();
    HardwareKey hardwareKey();
    void networkInterface(const std::string& interfaceName);
    std::string networkInterface();
    Hostname hostname();
    void preventSleep();
    void terminateProccess(int processId);
    int parentProcessId();
}
