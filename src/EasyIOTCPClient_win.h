#if  defined(WIN32) || defined(WIN64)
#ifndef EASYIOTCPCLIENT_H
#define EASYIOTCPCLIENT_H

#include "EasyIOTCPConnection_win.h"
#include "EasyIOIEventLoop.h"
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
            bool disconnect();

        private:
            Client(IEventLoopPtr worker);

        private:
            IEventLoopPtr m_worker;
            bool m_connecting;
            std::atomic<int> m_detained;
            void* m_connectEx ;
        };
    }
}

#endif
#endif
