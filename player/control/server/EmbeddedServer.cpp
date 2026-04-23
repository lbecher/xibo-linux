#include "EmbeddedServer.hpp"

#include "common/logger/Logging.hpp"

namespace
{
const std::string DefaultLocalAddress = "127.0.0.1";
const int DefaultThreadsCount = 2;
}

Session::Session(tcp::socket&& socket,
                 const FilePath& rootDirectory,
                 const InfoResponseFactory& infoFactory,
                 const CriteriaReceived& criteriaReceived,
                 const TriggerReceived& triggerReceived,
                 const DurationReceived& durationReceived,
                 const FaultReceived& faultReceived) :
    m_socket(std::move(socket)),
    send_(*this),
    infoController_(infoFactory),
    criteriaController_(criteriaReceived),
    hookController_(triggerReceived),
    durationController_(durationReceived),
    faultController_(faultReceived),
    restrictiveFileModule_(rootDirectory)
{
}

void Session::run()
{
    doRead();
}

void Session::doRead()
{
    request_ = {};
    http::async_read(m_socket, m_buffer, request_, std::bind(&Session::onRead, shared_from_this(), ph::_1, ph::_2));
}

void Session::onRead(beast::error_code ec, std::size_t /*bytesTransferred*/)
{
    if (ec == http::error::end_of_stream) return close();

    if (!ec)
    {
        HttpResponse response;

        if (infoController_.matches(request_))
        {
            response = infoController_.handle(request_);
        }
        else if (criteriaController_.matches(request_))
        {
            response = criteriaController_.handle(request_);
        }
        else if (hookController_.matches(request_))
        {
            response = hookController_.handle(request_);
        }
        else if (durationController_.matches(request_))
        {
            response = durationController_.handle(request_);
        }
        else if (faultController_.matches(request_))
        {
            response = faultController_.handle(request_);
        }
        else
        {
            response = restrictiveFileModule_.handle(request_);
        }

        std::visit([this](auto&& msg) { send_(std::move(msg)); }, std::move(response));
    }
    else
    {
        if (ec == net::error::connection_reset) return;
        Log::error("[EmbeddedServer] Read Error: {}", ec.message());
    }
}

void Session::onWrite(bool shouldBeClosed, beast::error_code ec, std::size_t /*bytesTransferred*/)
{
    if (!ec)
    {
        if (shouldBeClosed)
        {
            return close();
        }

        m_response = nullptr;
        doRead();
    }
    else
    {
        if (ec == net::error::connection_reset || ec == net::error::broken_pipe) return;
        Log::error("[EmbeddedServer] Write Error: {}", ec.message());
    }
}

void Session::close()
{
    beast::error_code ec;
    m_socket.shutdown(tcp::socket::shutdown_send, ec);
}

EmbeddedServer::EmbeddedServer() : work_(ioc_), acceptor_(ioc_)
{
    for (int i = 0; i != DefaultThreadsCount; ++i)
    {
        workerThreads_.push_back(std::make_unique<JoinableThread>([=]() {
            Log::trace("[EmbeddedServer] Thread started");
            ioc_.run();
        }));
    }
}

EmbeddedServer::~EmbeddedServer()
{
    ioc_.stop();
}

void EmbeddedServer::run(unsigned short port)
{
    try
    {
        tcp::endpoint endpoint(net::ip::address::from_string(DefaultLocalAddress), port);
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(net::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen(net::socket_base::max_listen_connections);
        port_ = port;

        doAccept();
    }
    catch (std::exception& e)
    {
        Log::error("[EmbeddedServer] Establish Error: {}", e.what());
    }
}

Uri EmbeddedServer::address() const
{
    return Uri::fromString("http://localhost:" + std::to_string(port_) + "/");
}

void EmbeddedServer::setRootDirectory(const FilePath& rootDirectory)
{
    rootDirectory_ = rootDirectory;
}

void EmbeddedServer::setInfoFactory(const InfoResponseFactory& factory)
{
    infoFactory_ = factory;
}

void EmbeddedServer::setCriteriaReceived(const CriteriaReceived& callback)
{
    criteriaReceived_ = callback;
}

void EmbeddedServer::setTriggerReceived(const TriggerReceived& callback)
{
    triggerReceived_ = callback;
}

void EmbeddedServer::setDurationReceived(const DurationReceived& callback)
{
    durationReceived_ = callback;
}

void EmbeddedServer::setFaultReceived(const FaultReceived& callback)
{
    faultReceived_ = callback;
}

void EmbeddedServer::doAccept()
{
    acceptor_.async_accept(ioc_, std::bind(&EmbeddedServer::onAccept, shared_from_this(), ph::_1, ph::_2));
}

void EmbeddedServer::onAccept(beast::error_code ec, tcp::socket socket)
{
    if (!ec)
    {
        std::make_shared<Session>(
            std::move(socket), rootDirectory_, infoFactory_, criteriaReceived_, triggerReceived_, durationReceived_, faultReceived_)
            ->run();
    }
    else
    {
        Log::error("[EmbeddedServer] Accept Connection Error: {}", ec.message());
    }

    doAccept();
}
