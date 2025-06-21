#pragma once
#include <lib/quantity.hpp>

namespace lib::units {
    template <class Length>
    struct Area
    {
        using Dimension = Degree<Length, 2>;
        constexpr static auto name() noexcept
        {
            return string("area");
        }
        constexpr static auto symbol() noexcept
        {
            return Length::symbol() + string("^2");
        }
    };

    template <class Length>
    struct Dimension<TDegree<Length, 2>>
    {
        using Type = Area<Length>;
    };

    template <char ...Chars>
    constexpr auto operator ""_m2() noexcept
    {
        using Parser = literal::Parser<Chars...>;
        return Quantity<Area<Metre>, typename Parser::Type>(Parser::value);
    }

    inline constexpr auto operator ""_m2(long double quantity) noexcept
    {
        return Quantity<Area<Metre>, long double>(quantity);
    }

}