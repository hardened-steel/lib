#pragma once
#include <lib/units.hpp>
#include <lib/literal.hpp>

namespace lib::units {
    struct ECurrent
    {
        using Dimension = ECurrent;
        constexpr static std::string_view name() noexcept
        {
            return "electric current";
        }
        constexpr static std::string_view symbol() noexcept
        {
            return "A";
        }
    };
    constexpr inline Unit<ECurrent> ampere {};

    template<char ...Chars>
    constexpr auto operator ""_A() noexcept
    {
        using Parser = literal::Parser<Chars...>;
        return Quantity<ECurrent, typename Parser::Type>(Parser::value);
    }

    inline constexpr auto operator ""_A(long double quantity) noexcept
    {
        return Quantity<ECurrent, long double>(quantity);
    }

}