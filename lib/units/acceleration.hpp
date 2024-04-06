#pragma once
#include <lib/units.hpp>
#include <lib/units/velocity.hpp>

namespace lib::units {
    template<class Length>
    struct Acceleration
    {
        using Dimension = Divide<Velocity<Length>, Time>;
        constexpr static auto name() noexcept
        {
            return string("acceleration");
        }
        constexpr static auto symbol() noexcept
        {
            return Length::symbol() + string("/s^2");
        }
    };

    template<class Length>
    struct Dimension<Multiplying<Velocity<Length>, TDegree<Time, 2>>>
    {
        using Type = Acceleration<Length>;
    };

    template<char ...Chars>
    constexpr auto operator ""_a() noexcept
    {
        using Parser = literal::Parser<Chars...>;
        return Quantity<Acceleration<Length>, typename Parser::Type>(Parser::value);
    }

    inline constexpr auto operator ""_a(long double quantity) noexcept
    {
        return Quantity<Acceleration<Length>, long double>(quantity);
    }
}
