#pragma once
#include <lib/units.hpp>
#include <lib/units/velocity.hpp>


namespace lib::units {
    struct Acceleration
    {
        using Dimension = Divide<Velocity, Second>;
        constexpr static auto name() noexcept
        {
            return string("acceleration");
        }
        constexpr static auto symbol() noexcept
        {
            return string("m/s^2");
        }
    };

    template <>
    struct Dimension<Multiplying<Metre, TDegree<Second, -2>>>
    {
        using Type = Acceleration;
    };

    template <char ...Chars>
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
