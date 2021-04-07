#ifndef EASYIOUDPISOCKET_H
#define EASYIOUDPISOCKET_H

#include "EasyIOAutoBuffer.h"
#include "EasyIODef.h"
#include <functional>

namespace EasyIO
{
    namespace UDP
    {
        class ISocket;
        typedef std::shared_ptr<ISocket> ISocketPtr ;

        class ISocket : public std::enable_shared_from_this<ISocket>
        {
        public:
            ISocket(){}
            virtual ~ISocket(){}

            virtual ISocketPtr share() = 0;
            virtual SOCKET handle() = 0;
            virtual bool send(const std::string& ip, unsigned short port, AutoBuffer buffer) = 0;
            virtual bool recv(AutoBuffer buffer) = 0;
            virtual void close() = 0;

            int lastSystemError(){ return m_lastSystemError; }

        protected:
            void setLastSystemError(int err) { m_lastSystemError = err; }

        public:
            std::function<void (ISocket*, const std::string& peerIP, unsigned short peerPort,
                                AutoBuffer data)> onBufferReceived;

        protected:
            int m_lastSystemError = 0;
        };
    }
}

#endif
