#pragma once

#include "cms/xmds/BaseRequestSerializer.hpp"
#include "cms/xmds/BaseResponseParser.hpp"
#include "cms/xmds/Soap.hpp"

#include "common/SoapField.hpp"

namespace GetData
{
    struct Result
    {
        std::string data;
    };

    struct Request
    {
        SoapField<std::string> serverKey{"serverKey"};
        SoapField<std::string> hardwareKey{"hardwareKey"};
        SoapField<int> widgetId{"widgetId"};
    };
}

template <>
class Soap::RequestSerializer<GetData::Request> : public BaseRequestSerializer<GetData::Request>
{
public:
    RequestSerializer(const GetData::Request& request);
    std::string string();
};

template <>
class Soap::ResponseParser<GetData::Result> : public BaseResponseParser<GetData::Result>
{
public:
    ResponseParser(const std::string& soapResponse);

protected:
    GetData::Result parseBody(const XmlNode& node) override;
};
