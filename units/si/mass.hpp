#pragma once
#include <lib/units.hpp>
#include <lib/literal.hpp>

namespace lib::units {
    struct Mass
    {
        using Dimension = Mass;
        constexpr static std::string_view name() noexcept
        {
            return "mass";
        }
        constexpr static std::string_view symbol() noexcept
        {
            return "kg";
        }
    };
    constexpr inline Unit<Mass> kilogram {};

    template<char ...Chars>
    constexpr auto operator ""_kg() noexcept
    {
        using Parser = literal::Parser<Chars...>;
        return Quantity<Mass, typename Parser::Type>(Parser::value);
    }

    inline constexpr auto operator ""_kg(long double quantity) noexcept
    {
        return Quantity<Mass, long double>(quantity);
    }

}