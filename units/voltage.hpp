#pragma once
#include <lib/quantity.hpp>

namespace lib::units {
    struct Voltage
    {
        //kg⋅m2⋅s−3⋅A−2
        using Dimension = Devide<Multiply<Mass, Length, Length>, Multiply<Time, Time, Time, ECurrent>>;
        constexpr static std::string_view name() noexcept
        {
            return "voltage";
        }
        constexpr static std::string_view symbol() noexcept
        {
            return "V";
        }
    };

    template<>
    struct Dimension<Voltage::Dimension>
    {
        using Type = Voltage;
    };

    template<char ...Chars>
    constexpr auto operator ""_V() noexcept
    {
        using Parser = literal::Parser<Chars...>;
        return Quantity<Voltage, typename Parser::Type>(Parser::value);
    }

    inline constexpr auto operator ""_V(long double quantity) noexcept
    {
        return Quantity<Voltage, long double>(quantity);
    }

}