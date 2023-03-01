#pragma once
#include "lib/units.hpp"
#include "lib/units/si/length.hpp"
#include <lib/quantity.hpp>

namespace lib::units {
    struct Velocity
    {
        using Dimension = Devide<Length, Time>;
        constexpr static auto name() noexcept
        {
            return StaticString("velocity");
        }
        constexpr static auto symbol() noexcept
        {
            return StaticString("m/s");
        }
    };

    template<>
    struct Dimension<Velocity::Dimension>
    {
        using Type = Velocity;
    };

}