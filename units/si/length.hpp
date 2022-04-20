#pragma once
#include <lib/units.hpp>
#include <lib/literal.hpp>

namespace lib::units {
    struct Length
    {
        using Dimension = Length;
        constexpr static std::string_view name() noexcept
        {
            return "length";
        }
        constexpr static std::string_view symbol() noexcept
        {
            return "m";
        }
    };
    constexpr inline Unit<Length> metre {};

    template<char ...Chars>
    constexpr auto operator ""_m() noexcept
    {
        using Parser = literal::Parser<Chars...>;
        return Quantity<Length, typename Parser::Type>(Parser::value);
    }

    inline constexpr auto operator ""_m(long double quantity) noexcept
    {
        return Quantity<Length, long double>(quantity);
    }

}