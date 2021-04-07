#ifdef __linux__
#ifndef EASYIOTCPCLIENT_H
#define EASYIOTCPCLIENT_H

#include "EasyIOTCPConnection_linux.h"
#include "EasyIOIEventLoop.h"
#include "EasyIOContext_win.h"
#include "EasyIOTCPIClient.h"
#include <atomic>

namespace EasyIO
{
    namespace TCP
    {
        class Client : public IClient, public Connection
        {
        public:
            static IClientPtr create();
            static IClientPtr create(IEventLoopPtr worker);

            ~Client();

            bool connect(const std::string& host, unsigned short port);

        private:
            Client(IEventLoopPtr worker);
            void close();

        private:
            IEventLoopPtr m_worker;
            bool m_connecting;
            std::atomic<int> m_detained;
        };
    }
}

#endif
#endif
