#pragma once

#include "cms/xmds/BaseRequestSerializer.hpp"
#include "cms/xmds/Soap.hpp"

#include "common/SoapField.hpp"

namespace GetDependency
{
    struct Request
    {
        SoapField<std::string> serverKey{"serverKey"};
        SoapField<std::string> hardwareKey{"hardwareKey"};
        SoapField<std::string> fileType{"fileType"};
        SoapField<std::string> id{"id"};
        SoapField<std::size_t> chunkOffset{"chunkOffset"};
        SoapField<std::size_t> chunkSize{"chuckSize"};
    };
}

template <>
class Soap::RequestSerializer<GetDependency::Request> : public BaseRequestSerializer<GetDependency::Request>
{
public:
    RequestSerializer(const GetDependency::Request& request);
    std::string string();
};
