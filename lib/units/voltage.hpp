#pragma once
#include <lib/quantity.hpp>

namespace lib::units {
    struct Voltage
    {
        //kg⋅m2⋅s−3⋅A−2
        using Dimension = Devide<Multiply<Mass, Degree<Length, 2>>, Multiply<Degree<Time, 3>, ECurrent>>;
        constexpr static auto name() noexcept
        {
            return StaticString("voltage");
        }
        constexpr static auto symbol() noexcept
        {
            return StaticString("V");
        }
    };

    template<>
    struct Dimension<Voltage::Dimension>
    {
        using Type = Voltage;
    };

    template<char ...Chars>
    constexpr auto operator ""_V() noexcept
    {
        using Parser = literal::Parser<Chars...>;
        return Quantity<Voltage, typename Parser::Type>(Parser::value);
    }

    inline constexpr auto operator ""_V(long double quantity) noexcept
    {
        return Quantity<Voltage, long double>(quantity);
    }

}