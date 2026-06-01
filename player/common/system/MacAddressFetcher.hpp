#pragma once

#include "common/PlayerRuntimeError.hpp"
#include "common/system/MacAddress.hpp"

#include <net/if.h>
#include <string>

using SocketDescriptor = int;
using InterfaceFlags = short int;

class MacAddressFetcher
{
public:
    class Error : public PlayerRuntimeError
    {
        using PlayerRuntimeError::PlayerRuntimeError;
    };

    static MacAddress fetch();
    static MacAddress fetch(const std::string& interfaceName);

private:
    static SocketDescriptor openSocket();
    static ifconf getInterfaceConfig(SocketDescriptor socket, char* buffer);
    static ifreq findInterface(SocketDescriptor socket, ifconf& config);
    static ifreq findInterface(SocketDescriptor socket, ifconf& config, const std::string& interfaceName);

    static std::string retrieveMacAddress(SocketDescriptor socket, ifreq& interfaceRequest);
    static InterfaceFlags retrieveFlags(SocketDescriptor socket, ifreq& interfaceRequest);
    static bool isNotLoopback(InterfaceFlags flags);
    static bool isConnected(InterfaceFlags flags);
};
