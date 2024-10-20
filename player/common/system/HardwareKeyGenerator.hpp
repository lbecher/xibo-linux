#pragma once

#include "common/system/HardwareKey.hpp"

#include <ios>
#include <regex>
#include <string>

class HardwareKeyGenerator
{
public:
    static HardwareKey generate();
};
