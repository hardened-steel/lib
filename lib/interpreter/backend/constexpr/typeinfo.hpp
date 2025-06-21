#pragma once
#include <lib/interpreter/backend/constexpr/root.hpp>
#include <lib/static.string.hpp>

namespace lib::interpreter::const_expr {

    template <class T>
    struct TypeTrait;

    template <>
    struct TypeTrait<std::uint32_t>
    {
        constexpr static inline auto name = lib::StaticString("u32");
        constexpr static inline auto size = sizeof(std::uint32_t);
        constexpr static inline auto save = [](auto& memory, Pointer sp, std::uint32_t value) {
            memory.save(sp, value);
        };
        constexpr static inline auto load = [](auto& memory, Pointer sp) {
            return memory.load(sp);
        };
    };
}
