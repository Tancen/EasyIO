#if  defined(WIN32) || defined(WIN64)
#ifndef EASYIOTCPCONNECTION_H
#define EASYIOTCPCONNECTION_H

#include <string>
#include <functional>
#include "EasyIOAutoBuffer.h"
#include "EasyIOContext_win.h"
#include <set>
#include <mutex>
#include <atomic>
#include "EasyIOTCPIConnection.h"

namespace EasyIO
{
    namespace TCP
    {
        class Connection : virtual public IConnection
        {
        public:
            Connection();
            Connection(SOCKET sock, bool connected = true);
            ~Connection();

            IConnectionPtr share();

            SOCKET handle();

            bool connected();

            bool disconnect();
            bool send(AutoBuffer buffer, bool completely, int *numPending);
            bool recv(AutoBuffer buffer, bool completely, int *numPending);

            bool enableKeepalive(unsigned long interval = 1000, unsigned long time = 2000);
            bool disableKeepalive();

            bool setSendBufferSize(unsigned long size);
            bool setReceiveBufferSize(unsigned long size);

            bool setLinger(unsigned short onoff, unsigned short linger);

            const std::string& localIP() const ;
            unsigned short localPort() const ;

            const std::string& peerIP() const ;
            unsigned short peerPort() const ;

            bool updateEndPoint();

            void bindUserdata(void* userdata);
            void* userdata() const ;

        protected:
            bool post(Context *context, bool isSend);
            bool post(AutoBuffer buffer, bool isSend, bool completely,
                    std::function<void(Context*, size_t)> doneCallback,
                    std::function<void(Context*, int)> errorCallback);

            void whenDone(Context *context, size_t increase, bool isSend,
                std::function<void (Connection*, AutoBuffer data)> callback);
            void whenSendDone(Context *context, size_t increase);
            void whenRecvDone(Context *context, size_t increase);
            void whenError(Context *context, int err);

            int increasePostCount();
            int decreasePostCount();

        protected:
            SOCKET m_handle;

            bool m_connected;
            std::atomic<bool> m_disconnecting;

            std::string m_localIP;
            unsigned short m_localPort;

            std::string m_peerIP;
            unsigned short m_peerPort;

            std::recursive_mutex m_lock;
            void* m_userdata;

            std::atomic<int> m_countPost;
        };
    }

}

#endif
#endif
