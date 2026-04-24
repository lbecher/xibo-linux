#pragma once

#include "common/JoinableThread.hpp"
#include "common/types/Uri.hpp"

#include <boost/signals2/signal.hpp>

#include <atomic>
#include <memory>
#include <string>

namespace Ws
{
    using MessageReceived = boost::signals2::signal<void(const std::string&)>;

    class Subscriber
    {
    public:
        ~Subscriber();

        void run(const Uri& uri, const std::string& cmsKey, const std::string& channel);
        void stop();

        MessageReceived& messageReceived();

    private:
        void runLoop(const Uri& uri, const std::string& cmsKey, const std::string& channel);
        bool sleepBeforeReconnect(int seconds);

        std::atomic_bool running_{false};
        std::atomic_int activeSocketFd_{-1};
        std::unique_ptr<JoinableThread> worker_;
        MessageReceived messageReceived_;
    };
}
