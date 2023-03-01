#pragma once
#include <lib/units.hpp>
#include <lib/literal.hpp>

namespace lib::units {
    struct Temperature
    {
        using Dimension = Temperature;
        constexpr static auto name() noexcept
        {
            return StaticString("degree Kelvin");
        }
        constexpr static auto symbol() noexcept
        {
            return StaticString("K");
        }
    };
    constexpr inline Unit<Temperature> kelvin {};

    template<char ...Chars>
    constexpr auto operator ""_K() noexcept
    {
        using Parser = literal::Parser<Chars...>;
        return Quantity<Temperature, typename Parser::Type>(Parser::value);
    }

    inline constexpr auto operator ""_K(long double quantity) noexcept
    {
        return Quantity<Temperature, long double>(quantity);
    }

}