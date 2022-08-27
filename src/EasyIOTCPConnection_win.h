#if  defined(WIN32) || defined(WIN64)
#ifndef EASYIOTCPCONNECTION_H
#define EASYIOTCPCONNECTION_H

#include <string>
#include <functional>
#include "EasyIOByteBuffer.h"
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

            void disconnect();
            bool disconnecting();

            void send(ByteBuffer buffer);
            void recv(ByteBuffer buffer);
            size_t numBytesPending();

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
            int send0();
            int recv0();
            void disconnect0(bool requireDecrease, bool requireUnlock, const std::string& reason);

            bool addTask(Context *ctx, std::list<Context*>& dst);
            int doFirstTask(std::list<Context*>& tasks, std::function<int(Context*)> transmitter);
            void popFirstTask(std::list<Context*>& tasks);
            void cleanTasks(std::list<Context*>& tasks);

            void whenSendDone(Context *ctx, size_t increase);
            void whenRecvDone(Context *ctx, size_t increase);
            void whenError(Context *ctx, int err);

            int increasePostCount();
            int decreasePostCount();

        protected:
            SOCKET m_handle;

            bool m_connected;
            bool m_disconnecting;
            std::atomic<size_t> m_numBytesPending;

            std::string m_localIP;
            unsigned short m_localPort;

            std::string m_peerIP;
            unsigned short m_peerPort;

            std::recursive_mutex m_lock;
            void* m_userdata;

            std::atomic<int> m_countPost;
            std::list<Context*> m_tasksSend;
            std::list<Context*> m_tasksRecv;
        };
    }

}

#endif
#endif
