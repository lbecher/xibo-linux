#include "RequiredFilesDownloader.hpp"

#include "cms/xmds/XmdsFileDownloader.hpp"
#include "cms/xmds/XmdsRequestSender.hpp"
#include "common/storage/FileCache.hpp"
#include "networking/HttpClient.hpp"

RequiredFilesDownloader::RequiredFilesDownloader(XmdsRequestSender& xmdsRequestSender, FileCache& fileCache) :
    xmdsRequestSender_{xmdsRequestSender},
    fileCache_{fileCache},
    xmdsFileDownloader_{std::make_unique<XmdsFileDownloader>(xmdsRequestSender)}
{
}

RequiredFilesDownloader::~RequiredFilesDownloader() {}

bool RequiredFilesDownloader::onRegularFileDownloaded(const ResponseContentResult& result, const RegularFile& file)
{
    auto [error, fileContent] = result;
    if (!error)
    {
        Log::info("[RequiredFilesDownloader] Downloaded file: name='{}' size={} bytes", file.name(), fileContent.size());
        Log::info("[RequiredFilesDownloader] Expected hash='{}' | File size from server={}", 
                  static_cast<std::string>(file.hash()), file.size());
        
        if (fileContent.size() > 0)
        {
            // Log primeiro e último byte
            unsigned char first = fileContent[0];
            unsigned char last = fileContent[fileContent.size() - 1];
            Log::debug("[RequiredFilesDownloader] First byte=0x{:02x} Last byte=0x{:02x}", (int)first, (int)last);
        }
        
        fileCache_.save(file.name(), fileContent, file.hash());

        Log::debug("[{}] Downloaded", file.name());
        return true;
    }
    else
    {
        Log::error("[{}] Download error: {}", file.name(), error);
        return false;
    }
}

bool RequiredFilesDownloader::onResourceFileDownloaded(const ResponseContentResult& result, const ResourceFile& file)
{
    auto [error, fileContent] = result;
    if (!error)
    {
        Log::info("[RequiredFilesDownloader] Saving downloaded resource: {}", file.name());
        fileCache_.save(file.name(), fileContent, file.lastUpdate());

        Log::debug("[{}] Downloaded", file.name());
        return true;
    }
    else
    {
        Log::error("[{}] Download error: {}", file.name(), error);
        return false;
    }
}

DownloadResult RequiredFilesDownloader::downloadRequiredFile(const ResourceFile& file)
{
    Log::info("[RequiredFilesDownloader] Downloading resource via XMDS: layoutId={}, regionId={}, mediaId={}, name={}",
              file.layoutId(),
              file.regionId(),
              file.mediaId(),
              file.name());
    return xmdsRequestSender_.getResource(file.layoutId(), file.regionId(), file.mediaId())
        .then([this, file](auto future) {
            auto [error, result] = future.get();

            return onResourceFileDownloaded(ResponseContentResult{error, result.resource}, file);
        });
}

DownloadResult RequiredFilesDownloader::downloadRequiredFile(const RegularFile& file)
{
    if (file.downloadType() == RegularFile::DownloadType::HTTP)
    {
        Log::info("[RequiredFilesDownloader] Downloading file via HTTP: {} from {}", file.name(), file.url());
        auto uri = Uri::fromString(file.url());
        return HttpClient::instance().get(uri).then([this, file](boost::future<HttpResponseResult> future) {
            return onRegularFileDownloaded(future.get(), file);
        });
    }
    else
    {
        Log::info("[RequiredFilesDownloader] Downloading file via XMDS: id={}, remoteId={}, type={}, requestType={}, dependency={}, name={}, size={}",
                  file.id(),
                  file.remoteId(),
                  file.type(),
                  file.requestType(),
                  file.isDependency(),
                  file.name(),
                  file.size());
        return xmdsFileDownloader_->download(file.remoteId(), file.requestType(), file.size(), file.isDependency())
            .then([this, file](boost::future<XmdsResponseResult> future) {
                return onRegularFileDownloaded(future.get(), file);
            });
    }
}

bool RequiredFilesDownloader::shouldBeDownloaded(const RegularFile& file) const
{
    return !fileCache_.valid(file.name()) || !fileCache_.cached(file);
}

bool RequiredFilesDownloader::shouldBeDownloaded(const ResourceFile& file) const
{
    return !fileCache_.valid(file.name()) || !fileCache_.cached(file);
}
