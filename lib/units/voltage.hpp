#pragma once
#include <lib/quantity.hpp>
#include <lib/units/area.hpp>

namespace lib::units {
    struct Voltage
    {
        //kg⋅m2⋅s−3⋅A−1
        using Dimension = Multiply<Mass, Area<Metre>, Degree<Second, -3>, Degree<ECurrent, -1>>;
        constexpr static auto name() noexcept
        {
            return string("voltage");
        }
        constexpr static auto symbol() noexcept
        {
            return string("V");
        }
    };

    template <>
    struct Dimension<Voltage::Dimension>
    {
        using Type = Voltage;
    };

    template <char ...Chars>
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