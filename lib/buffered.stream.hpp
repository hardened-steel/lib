#pragma once
#if false
#include <lib/buffer.hpp>
#include <lib/channel.hpp>
#include <lib/buffered.channel.hpp>

namespace lib {
    template <class T, std::size_t N>
    class BufferedStream
    {

    public:

    };

    template <class T>
    class IStream
    {
        BufferedChannel<T, 8> i_buffers;
        BufferedChannel<T, 8> o_buffers;
    public:

    };

    template <class Buffers, class IStream>
    class StreamReceiver
    {
        Buffers& buffers;
        IStream& istream;
    public:


    };
}
#endif