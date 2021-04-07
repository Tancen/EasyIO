#if  defined(WIN32) || defined(WIN64)
#ifndef EASYIOUDPSOCKET_H
#define EASYIOUDPSOCKET_H

#include "EasyIOUDPISocket.h"
#include "EasyIOContext_win.h"
#include <functional>
#include <atomic>
#include <mutex>

namespace EasyIO
{
    namespace UDP
    {
        class Socket : public ISocket
        {
        protected:
            class ReceivetionContext : public ::EasyIO::Context
            {
            public:
                ReceivetionContext(AutoBuffer buffer);

                sockaddr_in* addr();
                int* addrLen();

            private:
                sockaddr_in m_addr;
                int m_addrLen;
            };

        public:
            Socket(SOCKET sock);
            ~Socket();

            ISocketPtr share();
            SOCKET handle();
            bool send(const std::string& ip, unsigned short port, AutoBuffer buffer);
            bool recv(AutoBuffer buffer);
            void close();

        protected:
            bool _recv(ReceivetionContext *context);
            void whenRecvDone(::EasyIO::Context *context, size_t increase);
            void whenError(::EasyIO::Context *context, int err);

            int increasePostCount();
            int decreasePostCount();


        protected:
            SOCKET m_handle;
            std::atomic<int> m_countPost;
            std::recursive_mutex m_lock;
        };
    }
}

#endif
#endif
