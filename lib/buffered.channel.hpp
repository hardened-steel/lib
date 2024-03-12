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
    class BufferedChannel: public IOChannel<T>
    {
        CycleBuffer<T, N> buffer;
        mutable Event ievent;
        mutable Event oevent;
    public:
        IEvent revent() const noexcept override
        {
            return ievent;
        }
        T urecv() noexcept override
        {
            auto value = buffer.recv();
            oevent.emit();
            return value;
        }
        bool rpoll() const noexcept override
        {
            return buffer.rpoll();
        }
        IEvent sevent() const noexcept override
        {
            return oevent;
        }
        void usend(T value) noexcept override
        {
            buffer.send(std::move(value));
            ievent.emit();
        }
        bool spoll() const noexcept override
        {
            return buffer.spoll() && !closed();
        }
    public:
        bool closed() const noexcept override
        {
            return closed_;
        }
        void close() noexcept override
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
    private:
        bool closed_ = false;
    };
}
