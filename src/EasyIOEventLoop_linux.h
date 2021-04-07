#ifdef __linux__
#ifndef EASYIOEVENTLOOP_H
#define EASYIOEVENTLOOP_H

#include <thread>
#include <sys/epoll.h>
#include <functional>
#include <mutex>
#include <vector>
#include "EasyIODef.h"
#include "EasyIOIEventLoop.h"
#include "EasyIOContext_linux.h"

namespace EasyIO
{
    class EventLoop : public IEventLoop
    {
        struct Task
        {
            std::function<void(void*)> callback;
            void* userdata;
        };
    public:
        static IEventLoopPtr create(int maxEvents = 1024);
        ~EventLoop();

        IEventLoopPtr share();

        bool add(int fd, Context::Context *context, int& err);
        bool modify(int fd, Context::Context *context, int& err);
        bool remove(int fd, Context::Context *context, int& err);

    private:
        EventLoop();
        void execute(int maxEvents);

    private:
        int m_handle;
        bool m_exit;
        std::thread m_thread;
        __pid_t m_pid;
    };

}
#endif
#endif
