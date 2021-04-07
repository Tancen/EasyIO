#ifndef EASYIOUDPISERVER_H
#define EASYIOUDPISERVER_H

#include "EasyIOAutoBuffer.h"
#include "EasyIOUDPISocket.h"
#include <functional>

namespace EasyIO
{
    namespace UDP
    {
        class IServer
        {
        public:
            IServer(){}
            virtual ~IServer(){}
            virtual bool send(const std::string& ip, unsigned short port, AutoBuffer buffer) = 0;
            virtual bool recv(AutoBuffer buffer) = 0;
            virtual bool open(unsigned short port) = 0;
            virtual void close() = 0;
            virtual bool opened() = 0;

            int lastSystemError(){ return m_lastSystemError; }

        protected:
            void setLastSystemError(int err) { m_lastSystemError = err; }

        public:
            std::function<void (ISocket*, const std::string& peerIP, unsigned short peerPort,
                                AutoBuffer data)> onBufferReceived;

        protected:
            int m_lastSystemError = 0;
        };

        typedef std::shared_ptr<IServer> IServerPtr;
    }
}

#endif
