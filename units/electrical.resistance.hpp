#pragma once
#include <lib/quantity.hpp>

namespace lib::units {
    struct EResistance
    {
        //kg⋅m2⋅s−3⋅A−2
        using Dimension = Devide<Multiply<Mass, Length, Length>, Multiply<Time, Time, Time, ECurrent, ECurrent>>;
        constexpr static auto name() noexcept
        {
            return StaticString("electrical resistance");
        }
        constexpr static auto symbol() noexcept
        {
            return StaticString("ohm");
        }
    };

    template<>
    struct Dimension<EResistance::Dimension>
    {
        using Type = EResistance;
    };

    template<char ...Chars>
    constexpr auto operator ""_ohm() noexcept
    {
        using Parser = literal::Parser<Chars...>;
        return Quantity<EResistance, typename Parser::Type>(Parser::value);
    }

    inline constexpr auto operator ""_ohm(long double quantity) noexcept
    {
        return Quantity<EResistance, long double>(quantity);
    }

}