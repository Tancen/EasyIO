#ifdef __linux__
#include <arpa/inet.h>
#include "EasyIOTCPClient_linux.h"
#include "EasyIOEventLoop_linux.h"
#include <functional>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

using namespace EasyIO::TCP;
using namespace std::placeholders;

Client::Client(IEventLoopPtr worker)
    : Connection(static_cast<EventLoop*>(worker.get())),
      m_worker(worker),
      m_connecting(false),
      m_detained(0)
{

}

void Client::close()
{
    ++m_detained;
    Connection::close();
    --m_detained;
}

IClientPtr Client::create()
{
    IEventLoopPtr worker = EventLoop::create();
    return create(worker);
}

IClientPtr Client::create(IEventLoopPtr worker)
{
    IClientPtr ret;
    if (!dynamic_cast<EventLoop*>(worker.get()))
        return ret;

    ret.reset(new Client(worker));
    return ret;
}

Client::~Client()
{
    disconnect();
    while (m_connected || m_detained || m_connecting)
        std::this_thread::sleep_for(std::chrono::microseconds(1));
}

bool Client::connect(const std::string& host, unsigned short port)
{
    std::lock_guard<std::recursive_mutex> guard(m_lock);
    if (m_connected || m_connecting)
        return false;

    setLastSystemError(0);

    do
    {
        m_handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (m_handle == INVALID_SOCKET)
            break;

        int flag = fcntl(m_handle, F_GETFL, 0);
        if (-1 == flag )
            break;
        fcntl(m_handle, F_SETFL, flag | O_NONBLOCK);

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(host.c_str());
        addr.sin_port = htons(port);

        if(::connect(m_handle, (sockaddr*)&addr, sizeof(addr)) != 0 && errno != EINPROGRESS)
            break;

        m_context.events = EPOLLOUT | EPOLLET;
        m_context.setCallback([this](uint32_t events)
        {
            do
            {
                if ((events & EPOLLOUT) && !(events & EPOLLERR))
                {
                    m_context.events = EPOLLIN;
                    EventLoop* w = (EasyIO::EventLoop*)(m_worker.get());
                    int err = 0;
                    if(!w->modify(m_handle, &m_context, err))
                    {
                        setLastSystemError(err);

                        break;
                    }
                    m_connected = true;
                    m_connecting = false;
                    updateEndPoint();
                    if (onConnected)
                        onConnected(this);

                    m_context.setCallback(std::bind(&Client::handleEvents, this, _1));
                }
                else
                {
                    int err = 0;
                    socklen_t l = sizeof(err);
                    if(getsockopt(m_handle, SOL_SOCKET, SO_ERROR, &err, &l) == 0)
                    {
                        setLastSystemError(err);
                    }
                    break;
                }

                return;
            } while (0);

            int err = 0;
            EventLoop* w = (EasyIO::EventLoop*)(m_worker.get());
            w->remove(m_handle, &m_context, err);
            closesocket(m_handle);
            ++m_detained;
            m_connecting = false;
            m_handle = INVALID_SOCKET;
            if (onConnectFailed)
                onConnectFailed(this);
            --m_detained;

        });

        int err = 0;
        EventLoop* w = (EasyIO::EventLoop*)(m_worker.get());
        if (!w->add(m_handle, &m_context, err))
        {
            setLastSystemError(err);
            break;
        }
        return true;
    }
    while(0);

    closesocket(m_handle);
    m_handle = INVALID_SOCKET;
    m_connecting = false;

    return false;
}

#endif



