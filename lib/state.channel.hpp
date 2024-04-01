#pragma once
#include <lib/channel.hpp>
#include <lib/lockfree/shared.state.hpp>

namespace lib {
    template<class T>
    class StateChannel: public IOChannel<T>
    {
        lockfree::SharedState<T> state;
        mutable lib::Event       ievent;
        mutable lib::NeverEvent  oevent;
    public:
        StateChannel(T value = T{}) noexcept(std::is_nothrow_move_assignable_v<T>)
        : state(std::move(value))
        {}
    public:
        void close() noexcept override
        {
            ievent.emit();
        }
        bool closed() const noexcept override
        {
            return false;
        }
    public:
        IEvent sevent() const noexcept override
        {
            return oevent;
        }
        void usend(T value) noexcept(std::is_nothrow_move_assignable_v<T>) override
        {
            state.put(std::move(value));
            ievent.emit();
        }
        bool spoll() const noexcept override
        {
            return true;
        }
    public:
        IEvent revent() const noexcept override
        {
            return ievent;
        }
        T urecv() noexcept(std::is_nothrow_move_constructible_v<T>) override
        {
            return state.get();
        }
        bool rpoll() const noexcept override
        {
            return state.updated();
        }
    public:
        void update(const T& value) noexcept(std::is_nothrow_copy_assignable_v<T>)
        {
            state.put(value);
            ievent.emit();
        }
        void update(T&& value) noexcept(std::is_nothrow_move_assignable_v<T>)
        {
            state.put(std::move(value));
            ievent.emit();
        }
    };
}
