#ifndef EASYIOTCPICLIENT_H
#define EASYIOTCPICLIENT_H

#include <memory>
#include <functional>
#include "EasyIOTCPIConnection.h"

namespace EasyIO
{
    namespace TCP
    {
        class IClient;
        typedef std::shared_ptr<IClient> IClientPtr;

        class IClient : virtual public IConnection
        {
        public:
            IClient(){}
            virtual ~IClient(){}

            virtual bool connect(const std::string& host, unsigned short port) = 0;

        public:
            std::function<void (IConnection*)> onConnected;
            std::function<void (IConnection*)> onConnectFailed;
        };
    }
}

#endif