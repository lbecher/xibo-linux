#include "HardwareKeyGenerator.hpp"

#include "common/Utils.hpp"
#include "common/system/System.hpp"
#include "common/logger/Logging.hpp"
#include "config/AppConfig.hpp"

#include <boost/process/child.hpp>
#include <boost/process/io.hpp>
#include <regex>

namespace bp = boost::process;

HardwareKey HardwareKeyGenerator::generate()
{
    std::string key = static_cast<std::string>(System::macAddress());
    return HardwareKey{Md5Hash::fromString(key)};
}