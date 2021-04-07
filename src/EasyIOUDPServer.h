#ifndef EASYIOUDPSERVER_WIN_H
#define EASYIOUDPSERVER_WIN_H

#include "EasyIOUDPISocket.h"
#include "EasyIOUDPIServer.h"
#include "EasyIOIEventLoop.h"
#include <atomic>
#include <mutex>

namespace EasyIO
{
    namespace UDP
    {
        class Server : virtual public IServer
        {
        public:
            static IServerPtr create();
            static IServerPtr create(IEventLoopPtr worker);

            ~Server();

            bool send(const std::string& ip, unsigned short port, AutoBuffer buffer);
            bool recv(AutoBuffer buffer);
            bool open(unsigned short port);
            void close();

            bool opened();

        protected:
            Server(IEventLoopPtr worker);

        protected:
            std::mutex m_mutex;
            ISocketPtr m_handle;
            IEventLoopPtr m_worker;
            bool m_opened;
        };
    }
}

#endif
