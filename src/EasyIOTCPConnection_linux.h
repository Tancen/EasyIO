#ifdef __linux__
#ifndef EASYIOTCPCONNECTION_H
#define EASYIOTCPCONNECTION_H

#include <string>
#include <functional>
#include "EasyIODef.h"
#include "EasyIOAutoBuffer.h"
#include "EasyIOContext_linux.h"
#include "EasyIOEventLoop_linux.h"
#include "EasyIOTCPIConnection.h"
#include <list>
#include <mutex>

namespace EasyIO
{
    namespace TCP
    {
        class Connection;
        class ICallback
        {
        public:
            virtual ~ICallback();
            virtual void callback() = 0;
        };

        class DisconnectedCallback : public ICallback
        {
        public:
            DisconnectedCallback(Connection* con);
            void callback();

        private:
            Connection* m_con;
        };

        class BufferSentCallback : public ICallback
        {
        public:
            BufferSentCallback(Connection* con, AutoBuffer data);
            void callback();

        private:
            Connection* m_con;
            AutoBuffer m_data;
        };

        class BufferReceiveCallback : public ICallback
        {
        public:
            BufferReceiveCallback(Connection* con, AutoBuffer data);
            void callback();

        private:
            Connection* m_con;
            AutoBuffer m_data;
        };

        class Connection : virtual public IConnection
        {
            class Task
            {
            public:
                Task(AutoBuffer data, bool completely);
                ~Task();

                AutoBuffer data();
                void increase(size_t progress);
                size_t progress();
                bool finished();
                bool completely();

            private:
                AutoBuffer m_data;
                size_t m_progress;
                bool m_completely;
            };

            typedef std::shared_ptr<Task> TaskPtr;

        public:
            Connection(EventLoop *worker);
            Connection(EventLoop *worker, SOCKET sock, bool connected = true);
            ~Connection();

            IConnectionPtr share();
            SOCKET handle();

            bool connected();

            bool disconnect();
            bool send(AutoBuffer buffer, bool completely);
            bool recv(AutoBuffer buffer, bool completely);

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
            void handleEvents(uint32_t events);
            virtual void close();
            bool _send(int& err);

            void dispatchCallbacks();

        protected:
            SOCKET m_handle;
            EventLoop* m_worker;
            Context::Context m_context;
            bool m_connected;

            std::string m_localIP;
            unsigned short m_localPort;

            std::string m_peerIP;
            unsigned short m_peerPort;

            std::recursive_mutex m_lock;

            std::list<TaskPtr> m_tasksSend;
            TaskPtr m_taskReceive;

            bool m_disconnecting;
            bool m_inSending;

            void* m_userdata;

            std::list<ICallback*> m_callbackList;
        };
    }
}

#endif
#endif
