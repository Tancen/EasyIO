#include "EasyIOAutoBuffer.h"
#include <stdio.h>
#include <string.h>

using namespace EasyIO;

AutoBuffer::AutoBuffer()
    : m_size(0),
      m_capacity(0)
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

void AutoBuffer::reset()
{
	m_sptrData.reset();
	m_size = 0;
    m_capacity = 0;
}

void AutoBuffer::reset(size_t capacity)
{
    if (capacity == 0)
    {
        m_sptrData.reset();
        m_capacity = 0;
    }
    else
    {
        m_sptrData.reset(new char[capacity], free);
        m_capacity = capacity;
    }

    m_size = 0;
}

void AutoBuffer::reset(const char *data, size_t size)
{
    if (size == 0)
        return;
        
    m_sptrData.reset(new char[size], free);
	memcpy(m_sptrData.get(), data, size);
    m_capacity = m_size = size;
}

bool AutoBuffer::fill(size_t offset, const char *data, size_t size)
{
    if (offset + size > m_capacity)
        return false;

    memcpy(m_sptrData.get() + offset, data, size);
    return true;
}

void AutoBuffer::resize(size_t size)
{
    if (size <= m_capacity)
        m_size = size;
    else
    {
        auto oldData = m_sptrData;
        auto oldSize = m_size;
        reset(size);
        memcpy(m_sptrData.get(), oldData.get(), oldSize);
        m_size = size;
    }
}

char* AutoBuffer::data()
{
	return m_sptrData.get();
}

size_t AutoBuffer::size() const
{
    return m_size;
}

size_t AutoBuffer::capacity() const
{
    return m_capacity;
}

void AutoBuffer::free(char *p)
{
	delete[] p;
}

