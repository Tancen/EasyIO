#if  defined(WIN32) || defined(WIN64)
#include "EasyIOTCPClient_win.h"
#include "EasyIOEventLoop_win.h"
#include "EasyIOContext_win.h"
#include <mstcpip.h>
#include <mswsock.h>
#include <WinBase.h>
#include <functional>

using namespace EasyIO::TCP;
using namespace std::placeholders;

Client::Client(IEventLoopPtr worker)
    :   m_worker(worker),
        m_connecting(false),
        m_detained(0),
        m_connectEx(nullptr)
{

}

IClientPtr Client::create()
{
    IEventLoopPtr worker = EventLoop::create();
    return create(worker);
}

IClientPtr Client::create(IEventLoopPtr worker)
{
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR)
    {
        return IClientPtr();
    }

    IClientPtr ret;
    if (!dynamic_cast<EventLoop*>(worker.get()))
        return ret;

    ret.reset(new Client(worker));
    return ret;
}

Client::~Client()
{
    disconnect();
    while(m_connected || m_countPost || m_detained || m_connecting)
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    WSACleanup();
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
        {
            setLastSystemError(GetLastError());
            break;
        }

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.S_un.S_addr = INADDR_ANY;
        addr.sin_port = 0;
        if (bind(m_handle, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
        {
            setLastSystemError(GetLastError());
            break;
        }

        int err = 0;
        if (!((EventLoop*)m_worker.get())->attach(m_handle, err))
        {
            setLastSystemError(err);
            break;
        }

        if (!m_connectEx)
        {
            GUID guid = WSAID_CONNECTEX;
            DWORD numBytes;

            if(WSAIoctl(m_handle, SIO_GET_EXTENSION_FUNCTION_POINTER,
                &guid, sizeof(guid), &m_connectEx,
                sizeof(m_connectEx), &numBytes, NULL, NULL) == SOCKET_ERROR)
            {
                setLastSystemError(GetLastError());
                break;
            }
        }

        Context *context = new Context(true);
        context->onDone = [this](Context*, size_t)
        {
            m_connected = true;
            decreasePostCount();
            m_connecting = false;

            setsockopt(m_handle, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
            updateEndPoint();

            if (onConnected)
                this->onConnected(this);
        };

        context->onError = [this](Context* , int err)
        { 
            setLastSystemError(err);
            closesocket(m_handle);
            m_handle = INVALID_SOCKET;
            ++m_detained;
            decreasePostCount();
            m_connecting = false;

            if (onConnectFailed)
                this->onConnectFailed(this);
            --m_detained;
        };

        {
            sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_addr.S_un.S_addr = inet_addr(host.c_str());
            addr.sin_port = htons(port);

            increasePostCount();
            if(!((LPFN_CONNECTEX)m_connectEx)(m_handle, (sockaddr*)&addr, sizeof(addr), NULL, 0, NULL, context))
            {
                int err = GetLastError();
                if (err != ERROR_IO_PENDING)
                {
                    setLastSystemError(err);
                    decreasePostCount();
                    m_connecting = false;
                    break;
                }
            }
        }

        return true;
    }
    while (0);

    closesocket(m_handle);
    m_handle = INVALID_SOCKET;
    m_connecting = false;
    return false;
}

bool Client::disconnect()
{
    ++m_detained;
    bool ret = Connection::disconnect();
    --m_detained;

    return ret;
}
#endif



