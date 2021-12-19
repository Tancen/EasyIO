#ifdef __linux__
#include <sys/socket.h>
#include "EasyIOTCPConnection_linux.h"
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <thread>
#include <assert.h>

using namespace EasyIO::TCP;
using namespace std::placeholders;

Connection::Connection(EventLoop *worker)
    : Connection(worker, INVALID_SOCKET, false)
{

}

Connection::Connection(EventLoop *worker, SOCKET sock, bool connected)
    : m_worker(worker),
      m_handle(sock),
      m_context(std::bind(&Connection::handleEvents, this, _1)),
      m_connected(connected),
      m_localPort(0),
      m_peerPort(0),
      m_userdata(NULL),
      m_disconnecting(false),
      m_numPending(0)
{
    if (m_handle != INVALID_SOCKET)
    {
        int err = 0;
        if(!m_worker->add(m_handle, &m_context, err))
            setLastSystemError(err);
    }
}

Connection::~Connection()
{
    disconnect();
    while (m_connected || m_disconnecting)
        std::this_thread::sleep_for(std::chrono::microseconds(1));
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

bool Connection::disconnect()
{
    if (!m_connected || m_disconnecting)
        return false;

    std::lock_guard<std::recursive_mutex> lockGuard(m_lock);
    if (!m_connected || m_disconnecting)
        return false;

    m_disconnecting = true;
    shutdown(m_handle, SHUT_RDWR);

    return true;
}

bool Connection::send(AutoBuffer buffer, bool completely, int *numPending)
{
    if (!buffer.capacity() || buffer.capacity() == buffer.size())
        return false;

    if (!m_connected)
        return false;

    bool ret = true;
    bool isEmpty;
    {
        std::lock_guard<std::recursive_mutex> lockGuard(m_lock);

        if (!m_connected)
            return false;

        isEmpty = m_tasksSend.empty();
        m_tasksSend.push_back(TaskPtr(new Task(buffer, completely)));
        m_numPending++;
    }

    if (isEmpty)
    {
        do
        {
            int err;
            if (!_send(err))
            {
                if (err == 0)
                {
                    disconnect();
                    ret = false;
                }
                else if (err < 0)
                {
                    if (errno == EAGAIN)
                    {
                        m_lock.lock();
                        if (!m_connected)
                        {
                            ret = false;
                            m_lock.unlock();
                            break;
                        }

                        m_context.events |= EPOLLOUT;
                        err = 0;
                        if(!m_worker->modify(m_handle, &m_context, err))
                        {
                            setLastSystemError(err);
                            disconnect();
                            ret = false;
                        }
                        m_lock.unlock();
                    }
                    else
                    {
                        setLastSystemError(errno);
                        disconnect();
                        ret = false;
                    }
                }
            }

            dispatchCallbacks();
        }
        while (0);
    }


    if (ret && numPending)
        *numPending = m_numPending;

    return ret;
}

bool Connection::recv(AutoBuffer buffer, bool completely, int *numPending)
{
    if (!buffer.capacity() || buffer.capacity() == buffer.size())
        return false;

    if (!m_connected || m_taskReceive.get())
        return false;

    {
        std::lock_guard<std::recursive_mutex> lockGuard(m_lock);

        if (!m_connected || m_taskReceive.get())
            return false;

        m_numPending++;
        m_taskReceive.reset(new Task(buffer, completely));

        if (!(m_context.events & EPOLLIN))
        {
            m_context.events |= EPOLLIN;
            int err = 0;
            if(!m_worker->modify(m_handle, &m_context, err))
            {
                setLastSystemError(err);
                disconnect();
                return false;
            }
        }
    }

    if (numPending)
        *numPending = m_numPending;

    return true;
}

bool Connection::enableKeepalive(unsigned long nInterval, unsigned long nTime)
{
    int onoff = 1;
    int keepaliveinterval = nInterval;
    int keepalivetime = nTime;

    if (setsockopt(m_handle, SOL_SOCKET, SO_KEEPALIVE, (void*)&onoff, sizeof(onoff))
        || setsockopt(m_handle, SOL_TCP, TCP_KEEPIDLE, (void*)&keepalivetime, sizeof(keepalivetime))
        || setsockopt(m_handle, SOL_TCP, TCP_KEEPINTVL, (void*)&keepaliveinterval, sizeof(keepaliveinterval)))
    {
        setLastSystemError(errno);
        return false;
    }

    return true;
}

bool Connection::disableKeepalive()
{
    int onoff = 0;

    int ret = setsockopt(m_handle, SOL_SOCKET, SO_KEEPALIVE, (void*)&onoff, sizeof(onoff));
    if (ret)
    {
        setLastSystemError(errno);
    }
    return !ret;
}

bool Connection::setSendBufferSize(unsigned long nSize)
{
    int ret = setsockopt(m_handle, SOL_SOCKET, SO_SNDBUF, (char*)&nSize, sizeof(unsigned long));
    if (ret)
    {
        setLastSystemError(errno);
    }
    return !ret;
}

bool Connection::setReceiveBufferSize(unsigned long nSize)
{
    int ret = setsockopt(m_handle, SOL_SOCKET, SO_RCVBUF, (char*)&nSize, sizeof(unsigned long));
    if (ret)
    {
        setLastSystemError(errno);
    }
    return !ret;
}

bool Connection::setLinger(unsigned short onoff, unsigned short linger)
{
    struct linger opt = {onoff, linger};
    int ret = setsockopt(m_handle, SOL_SOCKET, SO_LINGER, (char*)&opt, sizeof(opt));
    if (ret)
    {
        setLastSystemError(errno);
    }
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
    socklen_t len = sizeof(sockaddr_in);
    sockaddr_in addrLocal, addrPeer;

    if(getpeername(m_handle, (sockaddr*)&addrPeer, &len) == -1)
    {
        setLastSystemError(errno);
        return false;
    }

    m_peerIP = inet_ntoa(addrPeer.sin_addr);
    m_peerPort = ntohs(addrPeer.sin_port);

    if(getsockname(m_handle, (sockaddr*)&addrLocal, &len) == -1)
    {
        setLastSystemError(errno);
        return false;
    }

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

void Connection::handleEvents(uint32_t events)
{
    int ret;
    size_t offset, size;

    if (!m_connected)
        return;

    {
        std::lock_guard<std::recursive_mutex> lockGuard(m_lock);
        if (!m_connected)
            return;
    }

    if (events & EPOLLERR)
    {
        int err = 0;
        socklen_t l = sizeof(err);
        if(getsockopt(m_handle, SOL_SOCKET, SO_ERROR, &err, &l) == 0)
        {
            setLastSystemError(err);
        }
        close();
        return;
    }

    if (events & EPOLLIN)
    {
        std::lock_guard<std::recursive_mutex> lockGuard(m_lock);
        if (m_taskReceive.get())
        {
            offset = m_taskReceive->progress();
            size = m_taskReceive->data().capacity() - offset;
            ret = ::recv(m_handle,
                m_taskReceive->data().data() + offset, size, 0);
            if (ret <= 0)
            {
                setLastSystemError(errno);
                close();
                return;
            }
            m_taskReceive->increase(ret);

            if (!m_taskReceive->completely() || m_taskReceive->finished())
            {
                m_callbackList.push_back(new BufferReceiveCallback(this, m_taskReceive->data()));
                m_taskReceive.reset();
                m_numPending--;
            }
        }
    }

    if (events & EPOLLOUT)
    {
        int err;
        if (!_send(err))
        {
            if (err == 0 || (err < 0 && errno != EAGAIN))
            {
                setLastSystemError(errno);
                close();
                return;
            }
        }
    }

    dispatchCallbacks();

    {
        std::lock_guard<std::recursive_mutex> lockGuard(m_lock);
        uint32_t events = m_context.events;
        if (m_tasksSend.empty())
            events &= ~EPOLLOUT;

        if (!m_taskReceive.get())
            events &= ~EPOLLIN;

        if (m_context.events != events)
        {
            m_context.events = events;
            int err = 0;
            if(!m_worker->modify(m_handle, &m_context, err))
            {
                setLastSystemError(errno);
                close();
                return;
            }
        }
    }

}

void Connection::close()
{
    {
        std::lock_guard<std::recursive_mutex> lockGuard(m_lock);

        int err = 0;
        if(!m_worker->remove(m_handle, &m_context, err))
            setLastSystemError(errno);
        closesocket(m_handle);
        m_handle = INVALID_SOCKET;
        m_connected = false;
        m_disconnecting = false;
        m_tasksSend.clear();
        m_taskReceive.reset();
        m_numPending = 0;
        m_callbackList.push_back(new DisconnectedCallback(this));
    }

    dispatchCallbacks();
}

bool Connection::_send(int& err)
{
    bool ret = true;
    err = 0;
    do
    {
        TaskPtr task;
        std::lock_guard<std::recursive_mutex> guard(m_lock);
        if (!m_tasksSend.empty())
            task = m_tasksSend.front();
        if (!task)
            break;

        size_t offset = task->progress();
        size_t size = task->data().capacity() - offset;
        err = ::send(m_handle, task->data().data() + offset, size, 0);
        if (err <= 0)
        {
            ret = false;
            break;
        }

        task->increase(err);

        if (!task->completely() || task->finished())
        {
            m_tasksSend.pop_front();
            m_numPending--;
            m_callbackList.push_back(new BufferSentCallback(this, task->data()));
        }


    } while (1);

    return ret;
}

void Connection::dispatchCallbacks()
{
    do
    {
        ICallback* cb = nullptr;
        m_lock.lock();
        if (!m_callbackList.empty())
        {
            cb = m_callbackList.front();
            m_callbackList.pop_front();
        }
        m_lock.unlock();

        if (!cb)
            break;

        cb->callback();
        delete cb;
    } while (1);
}

Connection::Task::Task(AutoBuffer data, bool completely)
    : m_data(data),
      m_progress(data.size()),
      m_completely(completely)
{

}

Connection::Task::~Task()
{

}

EasyIO::AutoBuffer Connection::Task::data()
{
    return m_data;
}

void Connection::Task::increase(size_t progress)
{
    assert(progress);
    m_progress += progress;

    assert(m_progress <= m_data.capacity());
    m_data.resize(m_progress);
}

size_t Connection::Task::progress()
{
    return m_progress;
}

bool Connection::Task::finished()
{
    return m_progress == m_data.capacity();
}

bool Connection::Task::completely()
{
    return m_completely;
}

DisconnectedCallback::DisconnectedCallback(Connection *con)
    :   m_con(con)
{
    assert(con);
}

void DisconnectedCallback::callback()
{
    if (m_con->onDisconnected)
        m_con->onDisconnected(m_con);
}

BufferSentCallback::BufferSentCallback(Connection *con, AutoBuffer data)
    :   m_con(con), m_data(data)
{
    assert(con);
}

void BufferSentCallback::callback()
{
    if (m_con->onBufferSent)
        m_con->onBufferSent(m_con, m_data);
}

BufferReceiveCallback::BufferReceiveCallback(Connection *con, AutoBuffer data)
    :   m_con(con), m_data(data)
{
    assert(con);
}

void BufferReceiveCallback::callback()
{
    if (m_con->onBufferReceived)
        m_con->onBufferReceived(m_con, m_data);
}

ICallback::~ICallback()
{

}

#endif

