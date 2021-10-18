#if  defined(WIN32) || defined(WIN64)
#include "EasyIOTCPServer_win.h"
#include "EasyIOEventLoop_win.h"
#include <time.h>

using namespace EasyIO::TCP;
using namespace std::placeholders;

Server::Server(EventLoopGroupPtr workers)
    :   m_workers(workers),
        m_opened(false)
{

}

IServerPtr Server::create()
{
    IEventLoopPtr w = EventLoop::create();
    if (!w.get())
        return IServerPtr();

    std::vector<IEventLoopPtr> ws;
    ws.push_back(w);

    return create(EventLoopGroupPtr(new EventLoopGroup(ws)));
}

IServerPtr Server::create(EasyIO::EventLoopGroupPtr workers)
{
    const auto& ws = workers->getEventLoopGroup();
    for (const auto & it :ws)
    {
        if (!dynamic_cast<EventLoop*>(it.get()))
        {
            return IServerPtr();
        }
    }

    return IServerPtr(new Server(workers));
}

Server::~Server()
{
    close();
}

bool Server::open(unsigned short port, unsigned int backlog)
{
    std::lock_guard<std::mutex> guard(m_mutex);

    if (m_opened)
        return false;

    setLastSystemError(0);

    do
    {
        m_acceptor.reset(new Acceptor());
        m_acceptor->onAccepted = std::bind(&Server::addConnection, this, _1);
        int err = 0;
        if(!m_acceptor->accept(port, backlog, err))
        {
            setLastSystemError(err);
            break;
        }

        m_opened = true;
        return true;
    } while(0);

    m_acceptor.reset();
    return false;
}

void Server::close()
{
    std::lock_guard<std::mutex> guard(m_mutex);
    m_acceptor.reset();

    m_lockConnections.lock();
    auto connections = m_connections;
    m_lockConnections.unlock();

    for (auto it : connections)
        it.second->disconnect();

    while (1)
    {
        {
            std::lock_guard<std::recursive_mutex> lockGuard(m_lockConnections);
            if (m_connections.empty())
                break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    m_opened = false;
}

bool Server::opened()
{
    return m_opened;
}

void Server::addConnection(SOCKET sock)
{
    IConnectionPtr con(new Connection(sock, true));
    con->updateEndPoint();
    con->onBufferSent = onBufferSent;
    con->onBufferReceived = onBufferReceived;
    con->onDisconnected = std::bind(&Server::removeConnection, this, _1);

    EventLoop* w = (EventLoop*)m_workers->getNext();
    int err = 0;
    if(w->attach(sock, err))
    {
        {
            std::lock_guard<std::recursive_mutex> lockGuard(m_lockConnections);
            m_connections.insert(std::make_pair(con.get(), con));
        }

        if (onConnected)
            onConnected(con.get());
    }
    else
    {
        setLastSystemError(err);
        closesocket(sock);
    };
}

void Server::removeConnection(IConnection *con)
{
    if (onDisconnected)
        onDisconnected(con);

    {
        std::lock_guard<std::recursive_mutex> lockGuard(m_lockConnections);
        m_connections.erase(con);
    }
}
#endif
