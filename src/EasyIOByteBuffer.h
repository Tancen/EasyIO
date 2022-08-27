#ifndef EASYIOBYTEBUFFER_H
#define EASYIOBYTEBUFFER_H

#include <string>
#include <memory>

namespace EasyIO
{
    class ByteBuffer
    {
        struct PrivateData
        {
            std::shared_ptr<unsigned char> data;
            size_t readerIndex = 0;
            size_t writerIndex = 0;
            size_t capacity = 0;
        };

    public:
        ByteBuffer();
        ByteBuffer(size_t capacity);
        ByteBuffer(const char *data, size_t size);
        ~ByteBuffer();

        char* head();
        unsigned char* uhead();

        char* data();
        unsigned char* udata();
        size_t capacity();

        size_t readerIndex();
        size_t writerIndex();

        size_t readableBytes();
        size_t read(char* dst, size_t len);
        size_t read(unsigned char* dst, size_t len);
        size_t get(char* dst, size_t len);
        size_t get(unsigned char* dst, size_t len);
        void write(ByteBuffer data, bool discardReadBytes = false);
        void write(const char* data, size_t len);
        void write(const unsigned char* data, size_t len);
        void write(char v);
        void write(unsigned char v);
        void write(int16_t v);
        void write(uint16_t v);
        void write(int32_t v);
        void write(uint32_t v);
        void write(int64_t v);
        void write(uint64_t v);
        void write(float v);
        void write(double v);

        void fill(char c, size_t len);
        size_t discardReadBytes(int len = -1);
        void moveReaderIndex(int offset);
        void moveWriterIndex(int offset);
        void clear();
        void reset(size_t capacity = 0);

        void ensureWritable(size_t bytes);
    private:
        size_t read(unsigned char* dst, size_t len, bool moveReaderIndex);

        static void free(unsigned char *p);

    private:
        std::shared_ptr<PrivateData> m_data;
    };
}

#endif
