#pragma once
#include <lib/units.hpp>
#include <lib/literal.hpp>

namespace lib::units {
    struct Metre
    {
        using Dimension = Metre;
        constexpr static auto name() noexcept
        {
            return string("metre");
        }
        constexpr static auto symbol() noexcept
        {
            return string("m");
        }
    };
    constexpr inline Unit<Metre> metre {};

    template<char ...Chars>
    constexpr auto operator ""_m() noexcept
    {
        using Parser = literal::Parser<Chars...>;
        return Quantity<Metre, typename Parser::Type>(Parser::value);
    }

    inline constexpr auto operator ""_m(long double quantity) noexcept
    {
        return Quantity<Metre, long double>(quantity);
    }

}