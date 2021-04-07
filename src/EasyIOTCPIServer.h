#ifndef EASYIOTCPISERVER_H
#define EASYIOTCPISERVER_H

#include <memory>
#include <functional>
#include "EasyIOTCPIConnection.h"

namespace EasyIO
{
    namespace TCP
    {
        class IServer;
        typedef std::shared_ptr<IServer> IServerPtr;

        class IServer
        {
        public:
            IServer(){}
            virtual ~IServer(){}

            virtual bool open(unsigned short port, unsigned int backlog = 15) = 0;
            virtual void close() = 0;
            virtual bool opened() = 0;

            int lastSystemError(){ return m_lastSystemError; }

        protected:
            void setLastSystemError(int err) { m_lastSystemError = err; }

        public:
            std::function<void (IConnection*)> onConnected;
            std::function<void (IConnection*)> onDisconnected;
            std::function<void (IConnection*, AutoBuffer data)> onBufferSent;
            std::function<void (IConnection*, AutoBuffer data)> onBufferReceived;

        protected:
            int m_lastSystemError = 0;
        };
    }
}


#endif
