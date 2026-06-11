#pragma once

#include "common/PlayerRuntimeError.hpp"
#include "common/system/MacAddress.hpp"

#include <string>

class MacAddressFetcher
{
public:
    class Error : public PlayerRuntimeError
    {
        using PlayerRuntimeError::PlayerRuntimeError;
    };

    static MacAddress fetch();
    static MacAddress fetch(const std::string& interfaceName);

};
