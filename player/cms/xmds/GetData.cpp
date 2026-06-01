#include "GetData.hpp"

#include "cms/xmds/Resources.hpp"

namespace Resources = XmdsResources::GetData;

Soap::RequestSerializer<GetData::Request>::RequestSerializer(const GetData::Request& request) :
    BaseRequestSerializer(request)
{
}

std::string Soap::RequestSerializer<GetData::Request>::string()
{
    return createRequest(Resources::Name, request().serverKey, request().hardwareKey, request().widgetId);
}

Soap::ResponseParser<GetData::Result>::ResponseParser(const std::string& soapResponse) :
    BaseResponseParser(soapResponse)
{
}

GetData::Result Soap::ResponseParser<GetData::Result>::parseBody(const XmlNode& node)
{
    GetData::Result result;
    result.data = node.get<std::string>(Resources::Data);
    return result;
}
