#pragma once
#include <lib/typename.hpp>
#include <string>


namespace lib {
    template <>
    struct TypeName<std::string>
    {
        constexpr static inline auto name = "std::string";
    };
}
