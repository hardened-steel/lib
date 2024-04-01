#pragma once
#include <lib/tag.hpp>
#include <lib/array.hpp>

namespace lib::interpreter::const_expr {

    class Memory
    {
        lib::span<std::uint8_t> storage;
    public:
        constexpr Memory(lib::span<std::uint8_t> storage) noexcept
        : storage(storage)
        {}

        constexpr std::uint8_t load(std::uint32_t ptr, lib::tag_t<std::uint8_t>) const
        {
            return storage[ptr];
        }
        constexpr void save(std::uint32_t ptr, std::uint8_t value)
        {
            storage[ptr] = value;
        }

        constexpr std::uint16_t load(std::uint32_t ptr, lib::tag_t<std::uint16_t>) const
        {
            std::uint16_t value = 0;
            value |= (static_cast<std::uint16_t>(storage[ptr + 0]) << 0u);
            value |= (static_cast<std::uint16_t>(storage[ptr + 1]) << 8u);
            return value;
        }
        constexpr void save(std::uint32_t ptr, std::uint16_t value)
        {
            storage[ptr + 0] = static_cast<std::uint8_t>(value >> 0u);
            storage[ptr + 1] = static_cast<std::uint8_t>(value >> 8u);
        }

        constexpr std::uint32_t load(std::uint32_t ptr, lib::tag_t<std::uint32_t>) const
        {
            std::uint32_t value = 0;
            value |= (static_cast<std::uint16_t>(storage[ptr + 0]) << 0u);
            value |= (static_cast<std::uint16_t>(storage[ptr + 1]) << 8u);
            value |= (static_cast<std::uint16_t>(storage[ptr + 2]) << 16u);
            value |= (static_cast<std::uint16_t>(storage[ptr + 3]) << 24u);
            return value;
        }
        constexpr void save(std::uint32_t ptr, std::uint32_t value)
        {
            storage[ptr + 0] = static_cast<std::uint8_t>(value >> 0u);
            storage[ptr + 1] = static_cast<std::uint8_t>(value >> 8u);
            storage[ptr + 2] = static_cast<std::uint8_t>(value >> 16u);
            storage[ptr + 3] = static_cast<std::uint8_t>(value >> 24u);
        }

        template<class T>
        constexpr T pop(std::uint32_t& sp, lib::tag_t<T> tag) const
        {
            auto val = load(sp, tag);
            sp += sizeof(T);
            return val;
        }
        template<class T>
        constexpr void push(std::uint32_t& sp, T value)
        {
            save(sp - sizeof(T), value);
            sp -= sizeof(T);
        }
    };
}
