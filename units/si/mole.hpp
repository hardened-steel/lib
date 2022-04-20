#pragma once
#include <lib/units.hpp>
#include <lib/literal.hpp>

namespace lib::units {
    struct Mole
    {
        using Dimension = Mole;
        constexpr static std::string_view name() noexcept
        {
            return "amount of substance";
        }
        constexpr static std::string_view symbol() noexcept
        {
            return "mol";
        }
    };
    constexpr inline Unit<Mole> mole {};

    template<char ...Chars>
    constexpr auto operator ""_mol() noexcept
    {
        using Parser = literal::Parser<Chars...>;
        return Quantity<Mole, typename Parser::Type>(Parser::value);
    }

    inline constexpr auto operator ""_mol(long double quantity) noexcept
    {
        return Quantity<Mole, long double>(quantity);
    }

}