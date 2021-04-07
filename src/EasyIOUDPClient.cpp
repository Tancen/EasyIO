#include "EasyIOUDPClient.h"
#if  defined(WIN32) || defined(WIN64)
    #include "EasyIOEventLoop_win.h"
#else
    #include "EasyIOEventLoop_linux.h"
#endif

EasyIO::UDP::Client::Client(IEventLoopPtr worker)
    :   Server(worker)
{

}


EasyIO::UDP::IClientPtr EasyIO::UDP::Client::create()
{
    IEventLoopPtr worker = EventLoop::create();
    return create(worker);
}

EasyIO::UDP::IClientPtr EasyIO::UDP::Client::create(EasyIO::IEventLoopPtr worker)
{
#if  defined(WIN32) || defined(WIN64)
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR)
    {
        return IClientPtr();
    }
#endif

    IClientPtr ret;
    if (!dynamic_cast<EventLoop*>(worker.get()))
        return ret;

    ret.reset(new Client(worker));
    return ret;
}

EasyIO::UDP::Client::~Client()
{

}
