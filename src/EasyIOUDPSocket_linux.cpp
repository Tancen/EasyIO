#ifdef __linux__
#include "EasyIOUDPSocket_linux.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <thread>
#include <assert.h>

using namespace std::placeholders;

EasyIO::UDP::Socket::Socket(EasyIO::EventLoop *worker, SOCKET sock)
    :   m_handle(sock),
        m_worker(worker),
        m_context(std::bind(&Socket::handleEvents, this, _1)),
        m_inSending(false)
{
    m_context.events = EPOLLIN;
    int err = 0;
    if (!m_worker->add(m_handle, &m_context, err))
        setLastSystemError(err);
}

EasyIO::UDP::Socket::~Socket()
{
    close();
}

EasyIO::UDP::ISocketPtr EasyIO::UDP::Socket::share()
{
    return std::dynamic_pointer_cast<ISocket>(this->shared_from_this());
}

SOCKET EasyIO::UDP::Socket::handle()
{
    return m_handle;
}

bool EasyIO::UDP::Socket::send(const std::string &ip, unsigned short port,
    EasyIO::AutoBuffer buffer)
{
    bool ret = true;
    do
    {
        if (!buffer.capacity() || buffer.capacity() == buffer.size())
        {
            ret = false;
            break;;
        }

        {
            std::lock_guard<std::recursive_mutex> lockGuard(m_lock);

            sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = inet_addr(ip.c_str());
            addr.sin_port = htons(port);

            int size = buffer.capacity() - buffer.size();
            int err = ::sendto(m_handle, buffer.data() + buffer.size(), size, 0, (sockaddr*)&addr, sizeof(addr));
            if (err <= 0)
            {
                setLastSystemError(errno);
                ret = false;
            }
        }
    } while (0);

    return ret;
}

bool EasyIO::UDP::Socket::recv(EasyIO::AutoBuffer buffer)
{
    if (!buffer.capacity() || buffer.capacity() == buffer.size())
        return false;

    if (m_taskReceive.get())
        return false;

    {
        std::lock_guard<std::recursive_mutex> lockGuard(m_lock);

        if (m_taskReceive.get())
            return false;

        m_taskReceive.reset(new AutoBuffer(buffer));
    }

    return true;
}

void EasyIO::UDP::Socket::close()
{
    std::lock_guard<std::recursive_mutex> lockGuard(m_lock);
    int err = 0;
    m_worker->remove(m_handle, &m_context, err);
    closesocket(m_handle);
    m_handle = INVALID_SOCKET;
    m_taskReceive.reset();
}

void EasyIO::UDP::Socket::handleEvents(uint32_t events)
{
    if (events & EPOLLIN)
    {
        ReceivetionTaskPtr r;
        std::string ip;
        unsigned short port;

        {
            std::lock_guard<std::recursive_mutex> lockGuard(m_lock);
            if (m_taskReceive.get())
            {
                sockaddr_in addr;
                socklen_t len = sizeof(addr);
                size_t size = m_taskReceive->capacity() - m_taskReceive->size();
                int ret = ::recvfrom(m_handle, m_taskReceive->data() + m_taskReceive->size(), size,
                                0, (sockaddr*)&addr, &len);
                if (ret <= 0)
                {
                    setLastSystemError(errno);
                    return;
                }

                m_taskReceive->resize(m_taskReceive->size() + ret);

                ip = inet_ntoa(addr.sin_addr);
                port = ntohs(addr.sin_port);

                r = m_taskReceive;
                m_taskReceive.reset();
            }
        }

        if (r.get() && this->onBufferReceived)
            this->onBufferReceived(this, ip, port, *r);
    }
}

#endif
