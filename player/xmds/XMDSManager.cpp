#include "XMDSManager.hpp"
#include "SOAPManager.hpp"

const std::string DEFAULT_CLIENT_TYPE = "linux";

XMDSManager::XMDSManager(const std::string& host, const std::string& serverKey, const std::string& hardwareKey) :
    m_soapManager(std::make_unique<SOAPManager>(host)), m_serverKey(serverKey), m_hardwareKey(hardwareKey)
{
}

void XMDSManager::registerDisplay(int clientCode, const std::string& clientVersion, const std::string& displayName, RegisterDisplayCallback callback)
{
    RegisterDisplay::Request request;
    request.serverKey = m_serverKey;
    request.hardwareKey = m_hardwareKey;
    request.clientType = DEFAULT_CLIENT_TYPE;
    request.clientCode = clientCode;
    request.clientVersion = clientVersion; // WARNING get from player version (?)
    request.macAddress = "test"; // FIXME get from the system
    request.displayName = displayName;

    m_soapManager->sendRequest<RegisterDisplay::Response>(request, [=](const RegisterDisplay::Response& response){
        callback(response.status, response.playerSettings);
    });
}

void XMDSManager::requiredFiles(RequiredFilesCallback callback)
{
    RequiredFiles::Request request;
    request.serverKey = m_serverKey;
    request.hardwareKey = m_hardwareKey;

    m_soapManager->sendRequest<RequiredFiles::Response>(request, [=](const RequiredFiles::Response& response){
        callback(response.requiredFiles, response.requiredResources);
    });
}

void XMDSManager::getResource(int layoutId, int regionId, int mediaId, GetResourceCallback callback)
{
    GetResource::Request request;
    request.serverKey = m_serverKey;
    request.hardwareKey = m_hardwareKey;
    request.layoutId = layoutId;
    request.regionId = std::to_string(regionId);
    request.mediaId = std::to_string(mediaId);

    m_soapManager->sendRequest<GetResource::Response>(request, callback);
}
