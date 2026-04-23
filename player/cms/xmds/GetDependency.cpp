#include "GetDependency.hpp"

#include "cms/xmds/Resources.hpp"

namespace Resources = XmdsResources::GetDependency;

Soap::RequestSerializer<GetDependency::Request>::RequestSerializer(const GetDependency::Request& request) :
    BaseRequestSerializer(request)
{
}

std::string Soap::RequestSerializer<GetDependency::Request>::string()
{
    return createRequest(Resources::Name,
                         request().serverKey,
                         request().hardwareKey,
                         request().fileType,
                         request().id,
                         request().chunkOffset,
                         request().chunkSize);
}
