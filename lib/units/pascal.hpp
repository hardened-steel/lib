#pragma once
#include <lib/units/newton.hpp>
#include <lib/units/area.hpp>

namespace lib::units {
    struct Pascal
    {
        using Dimension = Divide<Newton, Area>;
        constexpr static auto name() noexcept
        {
            return string("pressure");
        }
        constexpr static auto symbol() noexcept
        {
            return string("Pa");
        }
    };
    constexpr inline Unit<Pascal> pascal {};

    template<>
    struct Dimension<Pascal::Dimension>
    {
        using Type = Pascal;
    };

    template<char ...Chars>
    constexpr auto operator ""_Pa() noexcept
    {
        using Parser = literal::Parser<Chars...>;
        return Quantity<Pascal, typename Parser::Type>(Parser::value);
    }

    inline constexpr auto operator ""_Pa(long double quantity) noexcept
    {
        return Quantity<Pascal, long double>(quantity);
    }

}