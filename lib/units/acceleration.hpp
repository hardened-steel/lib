#pragma once
#include "lib/units.hpp"
#include <lib/units/velocity.hpp>

namespace lib::units {
    struct Acceleration
    {
        using Dimension = Devide<Velocity, Time>;
        constexpr static auto name() noexcept
        {
            return StaticString("acceleration");
        }
        constexpr static auto symbol() noexcept
        {
            return StaticString("m/s^2");
        }
    };

    template<>
    struct Dimension<Acceleration::Dimension>
    {
        using Type = Acceleration;
    };

    template<char ...Chars>
    constexpr auto operator ""_a() noexcept
    {
        using Parser = literal::Parser<Chars...>;
        return Quantity<Acceleration, typename Parser::Type>(Parser::value);
    }

    inline constexpr auto operator ""_a(long double quantity) noexcept
    {
        return Quantity<Acceleration, long double>(quantity);
    }

}