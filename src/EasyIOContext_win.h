#if  defined(WIN32) || defined(WIN64)
#ifndef EASYIOCONTEXT_H
#define EASYIOCONTEXT_H

#include <Winsock2.h>
#include <memory>
#include <functional>
#include "EasyIOAutoBuffer.h"
#include <atomic>

namespace EasyIO
{
    class Context : public OVERLAPPED
    {
    public:
        Context(bool completely);
        Context(AutoBuffer buffer, bool completely);

        void increase();
        void decrease();

        void increaseProgress(size_t increase);
        void error(int err);
        size_t progress();

        AutoBuffer buffer();
        WSABUF* WSABuf();
        bool finished();
        bool completely();

    public:
        std::function<void(Context*, size_t increase)> onDone;
        std::function<void(Context*, int err)> onError;

    protected:
        ~Context();

    private:
        AutoBuffer m_buffer;
        std::atomic<int> m_ref;
        size_t m_progress;
        bool m_completely;
        WSABUF m_wsaBuffer;
    };
}

#endif
#endif
