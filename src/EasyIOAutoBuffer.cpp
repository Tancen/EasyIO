#include "EasyIOAutoBuffer.h"
#include <stdio.h>
#include <string.h>

using namespace EasyIO;

AutoBuffer::AutoBuffer()
    : m_data(new Data())
{

}

AutoBuffer::AutoBuffer(size_t capacity)
    :   AutoBuffer()
{
    reset(capacity);
}

AutoBuffer::AutoBuffer(const char *data, size_t size)
    :   AutoBuffer()
{
    reset(data, size);
}

AutoBuffer::~AutoBuffer()
{

}

void AutoBuffer::reset()
{
    m_data->data.reset();
    m_data->size = 0;
    m_data->capacity = 0;
}

void AutoBuffer::reset(size_t capacity)
{
    if (capacity == 0)
    {
        m_data->data.reset();
        m_data->capacity = 0;
    }
    else
    {
        m_data->data.reset(new char[capacity], free);
        m_data->capacity = capacity;
    }

    m_data->size = 0;
}

void AutoBuffer::reset(const char *data, size_t size)
{
    reset(size);
        
    memcpy(m_data->data.get(), data, size);
    m_data->size = size;
}

bool AutoBuffer::fill(size_t offset, const char *data, size_t size)
{
    if (offset + size > m_data->capacity)
        return false;

    memcpy(m_data->data.get() + offset, data, size);
    return true;
}

void AutoBuffer::resize(size_t size)
{
    if (size <= m_data->capacity)
        m_data->size = size;
    else
    {
        auto oldData = m_data->data;
        auto oldSize = m_data->size;
        reset(size);
        memcpy(m_data->data.get(), oldData.get(), oldSize);
        m_data->size = size;
    }
}

void AutoBuffer::append(const char *data, size_t len)
{
    if (m_data->size + len <= m_data->capacity)
    {
        memcpy(m_data->data.get() + m_data->size, data, len);
        m_data->size += len;
    }
    else
    {
        size_t capacity = m_data->capacity;
        while (capacity < m_data->size + len)
            capacity *= 2;

        size_t oldSize = m_data->size;
        resize(capacity);
        memcpy(m_data->data.get() + oldSize, data, len);
        m_data->size = oldSize + len;
    }
}

void AutoBuffer::extend(size_t capacity)
{
    if (capacity < m_data->capacity)
    {
        resize(capacity);
    }
    else
    {
        size_t old = m_data->size;
        resize(capacity);
        m_data->size = old;
    }
}

char* AutoBuffer::data()
{
    return m_data->data.get();
}

size_t AutoBuffer::size() const
{
    return m_data->size;
}

size_t AutoBuffer::capacity() const
{
    return m_data->capacity;
}

void AutoBuffer::free(char *p)
{
	delete[] p;
}

