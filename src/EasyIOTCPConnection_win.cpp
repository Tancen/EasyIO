#if  defined(WIN32) || defined(WIN64)
#include "EasyIOTCPConnection_win.h"
#include "EasyIOError.h"
#include <mstcpip.h>
#include <mswsock.h>
#include <WinBase.h>
#include <thread>

using namespace EasyIO::TCP;
using namespace std::placeholders;

Connection::Connection()
    : Connection(INVALID_SOCKET, false)
{

}

Connection::Connection(SOCKET sock, bool connected)
    : m_handle(sock),
      m_connected(connected),
      m_disconnecting(false),
      m_numBytesPending(0),
      m_localPort(0),
      m_peerPort(0),
      m_userdata(nullptr),
      m_countPost(0)
{

}

Connection::~Connection()
{
    disconnect();
    do
    {
        {
            std::lock_guard  g(m_lock);
            if (!m_connected && !m_countPost)
                break;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    } while (true);
}

IConnectionPtr Connection::share()
{
    return std::dynamic_pointer_cast<IConnection>(this->shared_from_this());
}

SOCKET Connection::handle()
{
    return m_handle;
}

bool Connection::connected()
{
    return m_connected;
}

void Connection::disconnect()
{
    disconnect0(false, false, Error::STR_FORCED_CLOSURE);
}

bool Connection::disconnecting()
{
    return m_disconnecting;
}

void Connection::disconnect0(bool requireDecrease, bool requireUnlock, const std::string& reason)
{
    IConnectionPtr holder;
    if (!this->weak_from_this().expired())
        holder = this->shared_from_this();;

    bool unlocked = false;
    do
    {
        std::lock_guard g(m_lock);
        if (m_disconnecting || !m_connected)
            break;

        m_disconnecting = true;
        closesocket(m_handle);
        m_handle = INVALID_SOCKET;
    }
    while(0);

    do
    {
        {
            std::lock_guard g(m_lock);
            if (!m_connected)
                break;

            if (requireDecrease)
            {
                if (m_countPost - 1 > 0)
                    break;
            }
            else if (m_countPost)
                break;

            m_connected = false;
            m_numBytesPending = 0;
            cleanTasks(m_tasksSend);
            cleanTasks(m_tasksRecv);
        }
        if (requireUnlock)
        {
            m_lock.unlock();
            unlocked = true;
        }


        if (onDisconnected)
            onDisconnected(this, reason);

        {
            std::lock_guard  g(m_lock);
            m_disconnecting = false;
        }
    } while (0);


    if (requireUnlock && !unlocked)
        m_lock.unlock();

    if (requireDecrease)
    {
        decreasePostCount();
    }

}

void Connection::send(ByteBuffer buffer)
{
    if (!buffer.readableBytes())
        return;

    int err = 0;
    {
        std::lock_guard g(m_lock);
        if (!m_connected)
            return;

        Context* ctx = new Context(buffer, Context::OUTBOUND);
        ctx->onDone = std::bind(&Connection::whenSendDone, this, _1, _2);
        ctx->onError = std::bind(&Connection::whenError, this, _1, _2);

        m_numBytesPending += buffer.readableBytes();
        bool isEmpty = addTask(ctx, m_tasksSend);
        if (isEmpty)
        {
            err = send0();
        }
        ctx->decrease();
    }
    if (err)
        disconnect0(false, false, Error::formatError(err));
}

int Connection::send0()
{
    int ret = doFirstTask(m_tasksSend, [this](Context* ctx)
    {
        DWORD flags = 0;
        int ret = WSASend(m_handle, ctx->WSABuf(), 1,  NULL, flags, ctx, NULL);
        if (ret == SOCKET_ERROR)
        {
            int err = GetLastError();
            if(err != WSA_IO_PENDING)
                return err;
        }
        return 0;
    });
    return ret;
}

void Connection::recv(ByteBuffer buffer)
{
    buffer.ensureWritable(4096);

    int err = 0;
    {
        std::lock_guard g(m_lock);
        if (!m_connected)
            return;

        Context* ctx = new Context(buffer, Context::INBOUND);
        ctx->onDone = std::bind(&Connection::whenRecvDone, this, _1, _2);
        ctx->onError = std::bind(&Connection::whenError, this, _1, _2);

        bool isEmpty = addTask(ctx, m_tasksRecv);
        if (isEmpty)
        {
            err = recv0();
        }
        ctx->decrease();
    }
    if (err)
        disconnect0(false, false, Error::formatError(err));
}

int Connection::recv0()
{
    int ret = doFirstTask(m_tasksRecv, [this](Context* ctx)
    {
        DWORD flags = 0;
        int ret = WSARecv(m_handle, ctx->WSABuf(), 1,  NULL, &flags, ctx, NULL);
        if (ret == SOCKET_ERROR)
        {
            int err = GetLastError();
            if(err != WSA_IO_PENDING)
                return err;
        }
        return 0;
    });
    return ret;
}

size_t Connection::numBytesPending()
{
    return m_numBytesPending;
}

bool Connection::enableKeepalive(unsigned long nInterval, unsigned long nTime)
{
    tcp_keepalive inKeepAlive;
    tcp_keepalive outKeepAlive;
    unsigned long numBytes = 0;

    memset(&inKeepAlive, 0, sizeof(inKeepAlive));
    memset(&outKeepAlive, 0, sizeof(outKeepAlive));

    inKeepAlive.onoff = 1;
    inKeepAlive.keepaliveinterval = nInterval;
    inKeepAlive.keepalivetime = nTime;

    int ret = WSAIoctl(m_handle, SIO_KEEPALIVE_VALS, (LPVOID)&inKeepAlive, sizeof(tcp_keepalive),
            (LPVOID)&outKeepAlive, sizeof(tcp_keepalive), &numBytes, NULL, NULL);

    return !ret;
}

bool Connection::disableKeepalive()
{
    tcp_keepalive inKeepAlive;
    tcp_keepalive outKeepAlive;
    unsigned long numBytes = 0;

    memset(&inKeepAlive, 0, sizeof(inKeepAlive));
    memset(&outKeepAlive, 0, sizeof(outKeepAlive));

    inKeepAlive.onoff = 0;

    int ret = WSAIoctl(m_handle, SIO_KEEPALIVE_VALS, (LPVOID)&inKeepAlive, sizeof(tcp_keepalive),
            (LPVOID)&outKeepAlive, sizeof(tcp_keepalive), &numBytes, NULL, NULL);

    return !ret;
}

bool Connection::setSendBufferSize(unsigned long nSize)
{
    int ret = setsockopt(m_handle, SOL_SOCKET, SO_SNDBUF, (char*)&nSize, sizeof(unsigned long));
    return !ret;
}

bool Connection::setReceiveBufferSize(unsigned long nSize)
{
    int ret = setsockopt(m_handle, SOL_SOCKET, SO_RCVBUF, (char*)&nSize, sizeof(unsigned long));
    return !ret;
}

bool Connection::setLinger(unsigned short onoff, unsigned short linger)
{
    struct linger opt = {onoff, linger};
    int ret = setsockopt(m_handle, SOL_SOCKET, SO_LINGER, (char*)&opt, sizeof(opt));
    return !ret;
}

const std::string& Connection::localIP() const
{
    return m_localIP;
}

unsigned short Connection::localPort() const
{
    return m_localPort;
}

const std::string& Connection::peerIP() const
{
    return m_peerIP;
}

unsigned short Connection::peerPort() const
{
    return m_peerPort;
}

bool Connection::updateEndPoint()
{
    int len = sizeof(sockaddr_in);
    sockaddr_in addrLocal, addrPeer;

    if(getpeername(m_handle, (sockaddr*)&addrPeer, &len) == SOCKET_ERROR)
        return false;

    m_peerIP = inet_ntoa(addrPeer.sin_addr);
    m_peerPort = ntohs(addrPeer.sin_port);

    if(getsockname(m_handle, (sockaddr*)&addrLocal, &len) == SOCKET_ERROR)
        return false;

    m_localIP = inet_ntoa(addrLocal.sin_addr);
    m_localPort = ntohs(addrLocal.sin_port);
    return true;
}

void Connection::bindUserdata(void *userdata)
{
    m_userdata = userdata;
}

void *Connection::userdata() const
{
    return m_userdata;
}

bool Connection::addTask(Context *ctx, std::list<Context *> &dst)
{
    bool ret = dst.empty();
    ctx->increase();
    dst.push_back(ctx);
    return ret;
}

int Connection::doFirstTask(std::list<Context*>& tasks, std::function<int(Context*)> transmitter)
{
    std::lock_guard g(m_lock);

    if (tasks.empty())
        return 0;

    increasePostCount();
    Context* ctx = tasks.front();
    int ret = transmitter(ctx);
    if (ret)
        decreasePostCount();
    return ret;
}

void Connection::popFirstTask(std::list<Context*>& tasks)
{
    std::lock_guard g(m_lock);
    if (tasks.empty())
        return;
    Context* ctx = tasks.front();
    tasks.pop_front();
    ctx->decrease();
}

void Connection::cleanTasks(std::list<Context *> &tasks)
{
    std::lock_guard g(m_lock);

    while (!tasks.empty())
        popFirstTask(tasks);
}

void Connection::whenSendDone(Context *ctx, size_t increase)
{
    bool broken = false;
    int err = 0;
    do
    {
        if (!increase)
        {
            broken = true;
            std::lock_guard g(m_lock);
            err = Error::getSocketError(m_handle);
            break;
        }

        if (ctx->finished())
            popFirstTask(m_tasksSend);

        err = send0();
        broken = err;
    } while (0);

    m_lock.lock();
    if (m_disconnecting)
    {
        disconnect0(true, true, Error::STR_FORCED_CLOSURE);
    }
    else if (broken)
    {
        disconnect0(true, true, Error::formatError(err));
    }
    else
    {
        decreasePostCount();
        m_lock.unlock();
    }
}
void Connection::whenRecvDone(Context *ctx, size_t increase)
{
    bool broken = false;
    int err = 0;
    do
    {
        if (!increase)
        {
            broken = true;
            std::lock_guard g(m_lock);
            err = Error::getSocketError(m_handle);
            break;
        }

        if (onBufferReceived)
        {
            onBufferReceived(this, ctx->buffer());
        }

        popFirstTask(m_tasksRecv);
        err = recv0();
        broken = err;
    } while (0);

    m_lock.lock();
    if (m_disconnecting)
    {
        disconnect0(true, true, Error::STR_FORCED_CLOSURE);
    }
    else if (broken)
    {
        disconnect0(true, true, Error::formatError(err));
    }
    else
    {
        decreasePostCount();
        m_lock.unlock();
    }
}

void Connection::whenError(Context *context, int err)
{
    std::string reason;
    {
        std::lock_guard g(m_lock);
        if (m_disconnecting)
            reason = Error::STR_FORCED_CLOSURE;
        else
            reason = Error::formatError(err);
    }

    disconnect0(true, false, reason);
}

int Connection::increasePostCount()
{
    int ret = ++m_countPost;
    return ret;
}

int Connection::decreasePostCount()
{
    int ret = --m_countPost;
    return ret;
}

#endif


