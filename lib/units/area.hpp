#pragma once
#include <lib/quantity.hpp>

namespace lib::units {
    struct Area
    {
        using Dimension = Degree<Length, 2>;
        constexpr static auto name() noexcept
        {
            return string("area");
        }
        constexpr static auto symbol() noexcept
        {
            return string("m^2");
        }
    };

    template<>
    struct Dimension<Area::Dimension>
    {
        using Type = Area;
    };

    template<char ...Chars>
    constexpr auto operator ""_m2() noexcept
    {
        using Parser = literal::Parser<Chars...>;
        return Quantity<Area, typename Parser::Type>(Parser::value);
    }

    inline constexpr auto operator ""_m2(long double quantity) noexcept
    {
        return Quantity<Area, long double>(quantity);
    }

}