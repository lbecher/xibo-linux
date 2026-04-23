#pragma once

#include "control/server/CriteriaController.hpp"
#include "control/server/DurationController.hpp"
#include "control/server/HookController.hpp"
#include "control/server/InfoController.hpp"
#include "control/server/FaultController.hpp"
#include "control/server/RestrictiveFileModule.hpp"

#include "common/JoinableThread.hpp"
#include "common/fs/FilePath.hpp"
#include "common/types/Uri.hpp"

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/config.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;
namespace ph = std::placeholders;

class Session : public std::enable_shared_from_this<Session>
{
    struct SendLambda
    {
        Session& self;

        explicit SendLambda(Session& self_) : self(self_) {}

        template <bool isRequest, class Body, class Fields>
        void operator()(http::message<isRequest, Body, Fields>&& msg) const
        {
            auto sp = std::make_shared<http::message<isRequest, Body, Fields>>(std::move(msg));
            self.m_response = sp;
            http::async_write(self.m_socket,
                              *sp,
                              std::bind(&Session::onWrite, self.shared_from_this(), sp->need_eof(), ph::_1, ph::_2));
        }
    };

public:
    Session(tcp::socket&& socket,
            const FilePath& rootDirectory,
            const InfoResponseFactory& infoFactory,
            const CriteriaReceived& criteriaReceived,
            const TriggerReceived& triggerReceived,
            const DurationReceived& durationReceived,
            const FaultReceived& faultReceived);

    void run();
    void close();

private:
    void doRead();
    void onRead(beast::error_code ec, std::size_t /*bytesTransferred*/);
    void onWrite(bool shouldBeClosed, beast::error_code ec, std::size_t /*bytesTransferred*/);

private:
    net::ip::tcp::socket m_socket;
    beast::flat_buffer m_buffer;
    HttpRequest request_;
    std::shared_ptr<void> m_response;
    SendLambda send_;
    InfoController infoController_;
    CriteriaController criteriaController_;
    HookController hookController_;
    DurationController durationController_;
    FaultController faultController_;
    RestrictiveFileModule restrictiveFileModule_;
};

class EmbeddedServer : public std::enable_shared_from_this<EmbeddedServer>
{
public:
    EmbeddedServer();
    ~EmbeddedServer();

    void run(unsigned short port);
    Uri address() const;
    void setRootDirectory(const FilePath& rootDirectory);
    void setInfoFactory(const InfoResponseFactory& factory);
    void setCriteriaReceived(const CriteriaReceived& callback);
    void setTriggerReceived(const TriggerReceived& callback);
    void setDurationReceived(const DurationReceived& callback);
    void setFaultReceived(const FaultReceived& callback);

private:
    void doAccept();
    void onAccept(beast::error_code ec, tcp::socket socket);

private:
    net::io_context ioc_;
    net::io_context::work work_;
    std::vector<std::unique_ptr<JoinableThread>> workerThreads_;
    unsigned short port_ = 0;
    tcp::acceptor acceptor_;
    FilePath rootDirectory_;
    InfoResponseFactory infoFactory_;
    CriteriaReceived criteriaReceived_;
    TriggerReceived triggerReceived_;
    DurationReceived durationReceived_;
    FaultReceived faultReceived_;
};
