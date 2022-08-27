#if  defined(WIN32) || defined(WIN64)
#include "EasyIOContext_win.h"
#include "assert.h"

using namespace EasyIO;

Context::Context(Flag flag)
    :   m_flag(flag),
        m_ref(0)
{
    this->hEvent = 0;
    this->Internal = 0;
    this->InternalHigh = 0;
    this->Offset = 0;
    this->OffsetHigh = 0;
    this->Pointer = 0;

    m_wsaBuffer.buf = NULL;
    m_wsaBuffer.len = 0;
    increase();
}

Context::Context(EasyIO::ByteBuffer buffer, Flag flag)
    :   Context(flag)
{
    m_buffer = buffer;

    if (m_flag == OUTBOUND)
    {
        m_wsaBuffer.buf = m_buffer.data();
        m_wsaBuffer.len = m_buffer.readableBytes();
    }
    else
    {
        m_wsaBuffer.buf = m_buffer.head() + m_buffer.writerIndex();
        m_wsaBuffer.len = m_buffer.capacity() - m_buffer.writerIndex();
    }
}

void Context::increase()
{
    ++m_ref;
}

void Context::decrease()
{
    if(!--m_ref)
        delete this;
}

void Context::increaseProgress(size_t increase)
{
    if (m_flag == OUTBOUND)
    {
        m_buffer.moveReaderIndex(increase);

        m_wsaBuffer.buf = m_buffer.data();
        m_wsaBuffer.len = m_buffer.readableBytes();
    }
    else
    {
        m_buffer.moveWriterIndex(increase);
        m_wsaBuffer.buf = m_buffer.head() + m_buffer.writerIndex();
        m_wsaBuffer.len = m_buffer.capacity() - m_buffer.writerIndex();
    }

    if (onDone)
        onDone(this, increase);
}

void Context::error(int err)
{
    if (onError)
        onError(this, err);
}

EasyIO::ByteBuffer Context::buffer()
{
    return m_buffer;
}

WSABUF *Context::WSABuf()
{
    return &m_wsaBuffer;
}

bool Context::finished()
{
    if (m_flag == OUTBOUND)
        return m_buffer.readableBytes() == 0;
    else
        return true;
}

Context::~Context()
{

}

#endif
