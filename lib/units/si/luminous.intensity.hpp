#pragma once
#include <lib/units.hpp>
#include <lib/literal.hpp>

namespace lib::units {
    struct LuminousIntensity
    {
        using Dimension = LuminousIntensity;
        constexpr static auto name() noexcept
        {
            return string("candela");
        }
        constexpr static auto symbol() noexcept
        {
            return string("cd");
        }
    };
    constexpr inline Unit<LuminousIntensity> candela {};

    template <char ...Chars>
    constexpr auto operator ""_cd() noexcept
    {
        using Parser = literal::Parser<Chars...>;
        return Quantity<LuminousIntensity, typename Parser::Type>(Parser::value);
    }

    inline constexpr auto operator ""_cd(long double quantity) noexcept
    {
        return Quantity<LuminousIntensity, long double>(quantity);
    }

}
