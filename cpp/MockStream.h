#ifndef _MOCKSTREAM_H_
#define _MOCKSTREAM_H_

#include <memory>
#include <string.h>

class MockStream
{
    std::unique_ptr<char[]> buffer;
    char *end;

public:
    class Accessor
    {
        friend MockStream;
        char *ptr = nullptr, *end = nullptr;
        inline Accessor(char* ptr, char* end): ptr(ptr), end(end) {}

    public:
        inline Accessor() = default;
        
        template<class T>
        bool write(const T& v)
        {
            static constexpr auto size = sizeof(T);
            if(size <= size_t(end - ptr))
            {
                memcpy(ptr, &v, size);
                ptr += size;
                return true;
            }

            return false;
        }

        template<class T>
        bool read(T& v)
        {
            static constexpr auto size = sizeof(T);
            if(size <= size_t(end - ptr))
            {
                memcpy(&v, ptr, size);
                ptr += size;
                return true;
            }

            return false;
        }

        bool skip(size_t size)
        {
            if(size <= size_t(end - ptr))
            {
                ptr += size;
                return true;
            }

            return false;
        }
    };

    inline auto access() {
        return Accessor(buffer.get(), end);
    }

    inline bool truncateAt(size_t offset)
    {
        auto start = buffer.get();

        if(offset < size_t(end - start))
        {
            end = start + offset;
            return true;
        }

        return false;
    }

    MockStream() = delete;
    MockStream(const MockStream&) = delete;
    MockStream(MockStream&&) = default;
    MockStream& operator =(MockStream&&) = default;
    inline MockStream(size_t size): buffer(new char[size]), end(buffer.get() + size) {};
};

#endif /* _MOCKSTREAM_H_ */
