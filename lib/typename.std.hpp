#pragma once
#include <string>
#include <map>
#include <lib/typename.hpp>

namespace lib {
    template<>
    struct TypeName<std::string>
    {
        constexpr static inline auto name = "std::string";
    };

    template<class T, std::size_t N>
    struct TypeName<std::array<T, N>>
    {
        constexpr static inline auto name = "std::array<" + type_name<T> + ", " + value_string<N> + ">";
    };
}