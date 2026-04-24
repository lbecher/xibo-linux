#include "xmr/ws/Subscriber.hpp"

#include "common/logger/Logging.hpp"
#include "common/parsing/Parsing.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>

#include <openssl/ssl.h>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <string>
#include <thread>

#include <sys/socket.h>
#include <unistd.h>

namespace net = boost::asio;
namespace ssl = boost::asio::ssl;
namespace beast = boost::beast;
namespace websocket = boost::beast::websocket;
using tcp = boost::asio::ip::tcp;

namespace
{
std::string normalizedScheme(const Uri::Scheme& scheme)
{
    auto normalized = static_cast<std::string>(scheme);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return std::tolower(ch);
    });
    return normalized;
}

std::string targetFrom(const Uri& uri)
{
    if (uri.path().empty())
    {
        return "/";
    }

    return uri.path();
}

std::string initPayload(const std::string& cmsKey, const std::string& channel)
{
    JsonNode node;
    node.put("type", "init");
    node.put("key", cmsKey);
    node.put("channel", channel);
    return Parsing::jsonToString(node);
}
} // namespace

Ws::Subscriber::~Subscriber()
{
    stop();
}

void Ws::Subscriber::run(const Uri& uri, const std::string& cmsKey, const std::string& channel)
{
    stop();

    running_ = true;
    worker_ = std::make_unique<JoinableThread>([this, uri, cmsKey, channel]() { runLoop(uri, cmsKey, channel); });
}

void Ws::Subscriber::stop()
{
    running_ = false;

    auto socketFd = activeSocketFd_.exchange(-1);
    if (socketFd >= 0)
    {
        ::shutdown(socketFd, SHUT_RDWR);
    }

    worker_.reset();
}

Ws::MessageReceived& Ws::Subscriber::messageReceived()
{
    return messageReceived_;
}

void Ws::Subscriber::runLoop(const Uri& uri, const std::string& cmsKey, const std::string& channel)
{
    auto reconnectDelay = 1;
    const auto scheme = normalizedScheme(uri.scheme());
    const auto host = static_cast<std::string>(uri.authority().host());
    const auto port = uri.authority().port().string();
    const auto target = targetFrom(uri);
    const auto payload = initPayload(cmsKey, channel);

    while (running_)
    {
        try
        {
            net::io_context ioc;
            tcp::resolver resolver{ioc};
            auto results = resolver.resolve(host, port);

            if (scheme == "ws")
            {
                websocket::stream<beast::tcp_stream> ws{ioc};
                beast::get_lowest_layer(ws).connect(results);
                activeSocketFd_ = beast::get_lowest_layer(ws).socket().native_handle();

                ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
                ws.handshake(host, target);
                ws.write(net::buffer(payload));

                while (running_)
                {
                    beast::flat_buffer buffer;
                    ws.read(buffer);
                    auto message = beast::buffers_to_string(buffer.cdata());
                    if (!message.empty())
                    {
                        messageReceived_(message);
                    }
                }

                beast::error_code ec;
                ws.close(websocket::close_code::normal, ec);
            }
            else if (scheme == "wss")
            {
                ssl::context sslContext{ssl::context::tls_client};
                sslContext.set_verify_mode(ssl::verify_none);

                websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws{ioc, sslContext};
                if (!SSL_set_tlsext_host_name(ws.next_layer().native_handle(), host.c_str()))
                {
                    throw std::runtime_error{"Failed to configure TLS SNI host"};
                }

                beast::get_lowest_layer(ws).connect(results);
                activeSocketFd_ = beast::get_lowest_layer(ws).socket().native_handle();

                ws.next_layer().handshake(ssl::stream_base::client);
                ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
                ws.handshake(host, target);
                ws.write(net::buffer(payload));

                while (running_)
                {
                    beast::flat_buffer buffer;
                    ws.read(buffer);
                    auto message = beast::buffers_to_string(buffer.cdata());
                    if (!message.empty())
                    {
                        messageReceived_(message);
                    }
                }

                beast::error_code ec;
                ws.close(websocket::close_code::normal, ec);
            }
            else
            {
                throw std::runtime_error{"Unsupported websocket scheme: " + scheme};
            }
        }
        catch (const std::exception& e)
        {
            if (running_)
            {
                Log::error("[XMR::WS] {}", e.what());
            }
        }

        activeSocketFd_ = -1;

        if (!sleepBeforeReconnect(reconnectDelay))
        {
            break;
        }
        reconnectDelay = std::min(30, reconnectDelay * 2);
    }
}

bool Ws::Subscriber::sleepBeforeReconnect(int seconds)
{
    for (int second = 0; second < seconds; ++second)
    {
        if (!running_)
        {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return running_;
}
