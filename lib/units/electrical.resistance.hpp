#pragma once
#include <lib/quantity.hpp>
#include <lib/units/area.hpp>
#include <lib/units/si/electric.current.hpp>

namespace lib::units {
    struct EResistance
    {
        //kg⋅m2⋅s−3⋅A−2
        using Dimension = Multiply<Mass, Area<Metre>, Degree<Second, -3>, Degree<ECurrent, -2>>;
        constexpr static auto name() noexcept
        {
            return string("electrical resistance");
        }
        constexpr static auto symbol() noexcept
        {
            return string("ohm");
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