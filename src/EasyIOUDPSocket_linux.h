#ifdef __linux__
#ifndef EASYIOUDPSOCKET_LINUX_H
#define EASYIOUDPSOCKET_LINUX_H

#include "EasyIOUDPISocket.h"
#include "EasyIOContext_linux.h"
#include "EasyIOEventLoop_linux.h"
#include <functional>
#include <atomic>
#include <mutex>
#include <list>

namespace EasyIO
{
    namespace UDP
    {
        class Socket : public ISocket
        {
            typedef std::shared_ptr<AutoBuffer> ReceivetionTaskPtr;

        public:
            Socket(EventLoop *worker, SOCKET sock);
            ~Socket();

            ISocketPtr share();
            SOCKET handle();
            bool send(const std::string& ip, unsigned short port, AutoBuffer buffer);
            bool recv(AutoBuffer buffer);
            void close();

        protected:
            void handleEvents(uint32_t events);

        protected:
            SOCKET m_handle;
            EventLoop* m_worker;

            ReceivetionTaskPtr m_taskReceive;

            Context::Context m_context;

            bool m_inSending;
            std::recursive_mutex m_lock;
        };
    }
}

#endif
#endif
