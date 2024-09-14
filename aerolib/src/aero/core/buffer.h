#ifndef _AERO_BUFFER_H_
#define _AERO_BUFFER_H_

#include <stdint.h>
#include <string.h>

namespace aero
{

struct Buffer
{
    void* data;
    uint64_t size;

    Buffer()
        : data(nullptr),
          size(0)
    {
    }

    Buffer(const void* data, uint64_t _size)
        : data((void*)data),
          size(_size)
    {
    }

    Buffer(const Buffer& other, uint64_t _size)
        : data(other.data),
          size(_size)
    {
    }

    static Buffer Copy(const Buffer& other)
    {
        Buffer buff;
        buff.Allocate(other.size);
        memcpy(buff.data, other.data, other.size);
        return buff;
    }

    static Buffer Copy(const void* data, uint64_t _size)
    {
        Buffer buff;
        buff.Allocate(_size);
        memcpy(buff.data, data, _size);
        return buff;
    }

    void Allocate(uint64_t _size)
    {
        if (data)
            delete[] (uint8_t*)data;
        
        data = nullptr;

        if (_size == 0)
            return;

        data = new uint8_t[_size];
        size = _size;
    }

    void Release()
    {
        if (data)
            delete[] (uint8_t*)data;
        data = nullptr;
        size = 0;
    }

    void Zero()
    {
        if (data)
            memset(data, 0, size);
    }

    operator bool() const { return data; }

    template <typename T> T* As() const { return (T*)data; }

    inline uint64_t GetSize() const { return size; }
};

} // namespace aerolib

#endif // _AERO_BUFFER_H_
