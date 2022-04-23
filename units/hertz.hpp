#pragma once
#include <lib/quantity.hpp>

namespace lib::units {
    struct Hertz
    {
        using Dimension = Degree<Time, -1>;
        constexpr static auto name() noexcept
        {
            return StaticString("frequency");
        }
        constexpr static auto symbol() noexcept
        {
            return StaticString("Hz");
        }
    };
    constexpr inline Unit<Hertz> hertz {};

    template<>
    struct Dimension<Hertz::Dimension>
    {
        using Type = Hertz;
    };

    template<char ...Chars>
    constexpr auto operator ""_Hz() noexcept
    {
        using Parser = literal::Parser<Chars...>;
        return Quantity<Hertz, typename Parser::Type>(Parser::value);
    }

    inline constexpr auto operator ""_Hz(long double quantity) noexcept
    {
        return Quantity<Hertz, long double>(quantity);
    }

}