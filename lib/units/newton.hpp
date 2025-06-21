#pragma once
#include <lib/quantity.hpp>

namespace lib::units {
    struct Newton
    {
        using Dimension = Multiply<Mass, Metre, Degree<Second, -2>>;
        constexpr static auto name() noexcept
        {
            return string("force");
        }
        constexpr static auto symbol() noexcept
        {
            return string("N");
        }
    };
    constexpr inline Unit<Newton> newton {};

    template <>
    struct Dimension<Newton::Dimension>
    {
        using Type = Newton;
    };

    template <char ...Chars>
    constexpr auto operator ""_N() noexcept
    {
        using Parser = literal::Parser<Chars...>;
        return Quantity<Newton, typename Parser::Type>(Parser::value);
    }

    inline constexpr auto operator ""_N(long double quantity) noexcept
    {
        return Quantity<Newton, long double>(quantity);
    }

}