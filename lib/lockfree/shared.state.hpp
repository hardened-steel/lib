#pragma once
#include <lib/typename.hpp>
#include <cstdint>
#include <atomic>

namespace lib::lockfree {

    template<class T>
    class SharedState
    {
        std::array<T, 3> states;
        std::atomic_uint8_t current {0};
        std::uint8_t reader = 1;
        std::uint8_t writer = 2;
    private:
        constexpr static inline std::uint8_t dirty = (std::uint8_t(1) << std::uint8_t(7));
    public:
        constexpr SharedState(T state = T{}) noexcept(std::is_nothrow_copy_constructible_v<T>)
        : states{state, state, state}
        {}
        SharedState(const SharedState&) = delete;
        SharedState& operator=(const SharedState&) = delete;
    public:
        const T& get() noexcept
        {
            if (updated()) {
                reader = current.exchange(reader) & ~dirty;
            }
            return states[reader];
        }
        bool updated() const noexcept
        {
            return (current.load(std::memory_order_relaxed) & dirty) != 0;
        }
        void put(const T& value) noexcept(std::is_nothrow_copy_assignable_v<T>)
        {
            states[writer] = value;
            writer = current.exchange(writer | dirty) & ~dirty;
        }
        void put(T&& value) noexcept(std::is_nothrow_move_assignable_v<T>)
        {
            states[writer] = std::move(value);
            writer = current.exchange(writer | dirty) & ~dirty;
        }
    };
}

namespace lib {
    template<class T>
    struct TypeName<lockfree::SharedState<T>>
    {
        constexpr static inline StaticString name = "lib::lockfree::SharedState<" + type_name<T> + ">";
    };
}
