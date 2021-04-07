#if  defined(WIN32) || defined(WIN64)
#include "EasyIOUDPSocket_win.h"
#include <thread>

using namespace std::placeholders;

EasyIO::UDP::Socket::ReceivetionContext::ReceivetionContext(
    EasyIO::AutoBuffer buffer)
    :   ::EasyIO::Context(buffer, false),
        m_addrLen(sizeof(sockaddr_in))
{

}

int *EasyIO::UDP::Socket::ReceivetionContext::addrLen()
{
    return &m_addrLen;
}

sockaddr_in *EasyIO::UDP::Socket::ReceivetionContext::addr()
{
    return &m_addr;
}

EasyIO::UDP::Socket::Socket(SOCKET sock)
    :   m_handle(sock),
        m_countPost(0)
{

}

EasyIO::UDP::Socket::~Socket()
{
    close();
    while(m_countPost)
        std::this_thread::sleep_for(std::chrono::microseconds(1));
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
                setLastSystemError(GetLastError());
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

    ReceivetionContext *context = new ReceivetionContext(buffer);
    context->onDone = std::bind(&Socket::whenRecvDone, this, _1, _2);
    context->onError = std::bind(&Socket::whenError, this, _1, _2);

    bool ret = _recv(context);
    context->decrease();
    return ret;
}

void EasyIO::UDP::Socket::close()
{
    closesocket(m_handle);
}

bool EasyIO::UDP::Socket::_recv(EasyIO::UDP::Socket::ReceivetionContext *context)
{
    context->increase();
    do
    {
        m_lock.lock();
        increasePostCount();

        int ret;
        DWORD flags = 0;
        ret = WSARecvFrom(m_handle, context->WSABuf(), 1,  NULL, &flags,
                            (sockaddr*)(context->addr()), context->addrLen(), context, NULL);
        m_lock.unlock();

        if (ret == SOCKET_ERROR)
        {
            int err = GetLastError();
            if(err != WSA_IO_PENDING)
            {
                setLastSystemError(err);
                decreasePostCount();
                break;
            }
        }

        return true;
    }while(0);

    context->decrease();
    return false;
}

void EasyIO::UDP::Socket::whenRecvDone(::EasyIO::Context *context, size_t increase)
{
    EasyIO::UDP::Socket::ReceivetionContext* cxt = (EasyIO::UDP::Socket::ReceivetionContext*)context;

    std::string ip = inet_ntoa(cxt->addr()->sin_addr);
    unsigned short port = ntohs(cxt->addr()->sin_port);
    if (onBufferReceived)
        onBufferReceived(this, ip, port, cxt->buffer());


    decreasePostCount();
}

void EasyIO::UDP::Socket::whenError(::EasyIO::Context *context, int err)
{
    setLastSystemError(err);
    decreasePostCount();
}

int EasyIO::UDP::Socket::increasePostCount()
{
    return ++m_countPost;
}

int EasyIO::UDP::Socket::decreasePostCount()
{
    return --m_countPost;
}

#endif

