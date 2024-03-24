#pragma once
#include <mutex>
#include <type_traits>
#include <lib/cycle.buffer.hpp>
#include <lib/channel.hpp>
#include <lib/mutex.hpp>

namespace lib {

    /// @todo Реализовать временный перенос приоритета потоков, т.е. если писатель имеет больший приоритет
    /// чем читатель и ему некуда писать новые данные, то приоритет читателя повышается до приоритета писателя.
    /// Нужно так же исправить класс lib::IRange<Channel>.
    template<class T, std::size_t N>
    class BufferedChannel: public IOChannel<BufferedChannel<T, N>>
    {
        CycleBuffer<T, N> buffer;
        std::atomic_bool closed_ = false;
        mutable Event ievent;
        mutable Event oevent;
    public:
        using Type = T;
        using REvent = Event;
        using SEvent = Event;
    public:
        Event& revent() const noexcept
        {
            return ievent;
        }
        T urecv() noexcept
        {
            auto value = buffer.recv();
            oevent.emit();
            return value;
        }
        bool rpoll() const noexcept
        {
            return buffer.rpoll();
        }
        Event& sevent() const noexcept
        {
            return oevent;
        }
        void usend(T value) noexcept
        {
            buffer.send(std::move(value));
            ievent.emit();
        }
        bool spoll() const noexcept
        {
            return buffer.spoll() && !closed();
        }
    public:
        bool closed() const noexcept
        {
            return closed_;
        }
        void close() noexcept
        {
            closed_ = true;
            ievent.emit();
            oevent.emit();
        }
        void open() noexcept
        {
            closed_ = false;
        }
        std::size_t rsize() const noexcept
        {
            return buffer.rsize();
        }
        std::size_t wsize() const noexcept
        {
            return buffer.wsize();
        }
        void clear() noexcept
        {
            buffer.clear();
            oevent.emit();
            ievent.emit();
        }
    };
}
