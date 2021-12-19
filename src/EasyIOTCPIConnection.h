#ifndef EASYIOTCPICONNECTION_H
#define EASYIOTCPICONNECTION_H

#include "EasyIOAutoBuffer.h"
#include "EasyIODef.h"
#include <functional>
#include <memory>

namespace EasyIO
{
    namespace TCP
    {
        class IConnection;
        typedef std::shared_ptr<IConnection> IConnectionPtr;

        class IConnection : public std::enable_shared_from_this<IConnection>
        {
        public:
            IConnection(){}
            virtual ~IConnection(){}

            virtual IConnectionPtr share() = 0;
            virtual SOCKET handle() = 0;

            virtual bool connected() = 0;

            virtual bool disconnect() = 0;
            virtual bool send(AutoBuffer buffer, bool completely = true, int *numPending = nullptr) = 0;
            virtual bool recv(AutoBuffer buffer, bool completely = true, int *numPending = nullptr) = 0;

            virtual bool enableKeepalive(unsigned long interval = 1000, unsigned long time = 2000) = 0;
            virtual bool disableKeepalive() = 0;

            virtual bool setSendBufferSize(unsigned long size) = 0;
            virtual bool setReceiveBufferSize(unsigned long size) = 0;

            virtual bool setLinger(unsigned short onoff, unsigned short linger) = 0;

            virtual const std::string& localIP() const = 0;
            virtual unsigned short localPort() const = 0;

            virtual const std::string& peerIP() const = 0;
            virtual unsigned short peerPort() const = 0;

            virtual bool updateEndPoint() = 0;

            virtual void bindUserdata(void* userdata) = 0;
            virtual void* userdata() const = 0;

            int lastSystemError(){ return m_lastSystemError; }

        protected:
            void setLastSystemError(int err) { m_lastSystemError = err; }

        public:
            std::function<void (IConnection*)> onDisconnected;
            std::function<void (IConnection*, AutoBuffer data)> onBufferSent;
            std::function<void (IConnection*, AutoBuffer data)> onBufferReceived;

        protected:
            int m_lastSystemError = 0;
        };
    }
}
#endif
