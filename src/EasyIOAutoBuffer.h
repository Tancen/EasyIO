#ifndef EASYIOAUTOBUFFER_H
#define EASYIOAUTOBUFFER_H

#include <string>
#include <memory>

namespace EasyIO
{
    class AutoBuffer
    {
        struct Data
        {
            std::shared_ptr<char> data;
            size_t size = 0;
            size_t capacity = 0;
        };

    public:
        AutoBuffer();
        AutoBuffer(size_t capacity);
        AutoBuffer(const char *data, size_t size);
        ~AutoBuffer();

        void reset();
        void reset(size_t capacity);
        void reset(const char *data, size_t size);

        bool fill(size_t offset, const char *data, size_t size);
        void resize(size_t size = 0);

        char* data();
        size_t size() const;
        size_t capacity() const;

    private:
        static void free(char *p);

    private:
        std::shared_ptr<Data> m_data;
    };
}

#endif
