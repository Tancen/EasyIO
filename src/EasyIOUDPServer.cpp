#include "EasyIOUDPServer.h"

#if  defined(WIN32) || defined(WIN64)
    #include "EasyIOUDPSocket_win.h"
    #include "EasyIOEventLoop_win.h"
#else
    #include "EasyIOUDPSocket_linux.h"
    #include "EasyIOEventLoop_linux.h"
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <fcntl.h>
#endif

EasyIO::UDP::IServerPtr EasyIO::UDP::Server::create()
{
    IEventLoopPtr worker = EventLoop::create();
    return create(worker);
}

EasyIO::UDP::IServerPtr EasyIO::UDP::Server::create(EasyIO::IEventLoopPtr worker)
{
    IServerPtr ret;
    if (!dynamic_cast<EventLoop*>(worker.get()))
        return ret;

    ret.reset(new Server(worker));
    return ret;
}

EasyIO::UDP::Server::~Server()
{
    close();
}

bool EasyIO::UDP::Server::send(const std::string &ip, unsigned short port, EasyIO::AutoBuffer buffer)
{
    m_mutex.lock();
    auto handle = m_handle;
    m_mutex.unlock();

    if (!handle.get())
        return false;

    return handle->send(ip, port, buffer);
}

bool EasyIO::UDP::Server::recv(EasyIO::AutoBuffer buffer)
{
    m_mutex.lock();
    auto handle = m_handle;
    m_mutex.unlock();

    if (!handle.get())
        return false;

    return handle->recv(buffer);
}

bool EasyIO::UDP::Server::open(unsigned short port)
{
    {
        std::lock_guard<std::mutex> guard(m_mutex);

        if (m_opened)
            return false;
        do
        {
            SOCKET sock  = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if (sock == INVALID_SOCKET)
            {
                #if  defined(WIN32) || defined(WIN64)
                    setLastSystemError(GetLastError());
                #else
                    setLastSystemError(errno);
                #endif
                break;
            }


            EventLoop* w = (EventLoop*)m_worker.get();
            #if  defined(WIN32) || defined(WIN64)
                m_handle.reset(new Socket(sock));
                m_handle->onBufferReceived = onBufferReceived;
                sockaddr_in addr;
                addr.sin_family = AF_INET;
                addr.sin_addr.S_un.S_addr = INADDR_ANY;
                addr.sin_port = htons(port);

                if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
                {
                    setLastSystemError(GetLastError());
                    break;
                }

                int err = 0;
                if(!w->attach(sock, err))
                {
                    setLastSystemError(err);
                    break;
                }
            #else
                m_handle.reset(new Socket(w, sock));
                m_handle->onBufferReceived = onBufferReceived;
                sockaddr_in addr;
                addr.sin_family = AF_INET;
                addr.sin_addr.s_addr = INADDR_ANY;
                addr.sin_port = htons(port);

                if(bind(sock, (sockaddr*)&addr, sizeof(addr)) == -1)
                {
                    setLastSystemError(errno);
                    break;
                }
            #endif

            m_opened = true;
            return true;
        } while(0);
    }

    m_handle.reset();
    return false;
}

void EasyIO::UDP::Server::close()
{
    ISocketPtr h;
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        h = m_handle;
        m_handle.reset();
        m_opened = false;
    }
}

bool EasyIO::UDP::Server::opened()
{
    return m_opened;
}

EasyIO::UDP::Server::Server(EasyIO::IEventLoopPtr worker)
    :   m_worker(worker),
        m_opened(false)
{

}

