#pragma once
#include <lib/units/si/length.hpp>
#include <lib/units/si/time.hpp>


namespace lib::units {
    struct Velocity
    {
        using Dimension = Divide<Metre, Second>;
        constexpr static auto name() noexcept
        {
            return string("velocity");
        }
        constexpr static auto symbol() noexcept
        {
            return string("m/s");
        }
    };

    template <>
    struct Dimension<Velocity::Dimension>
    {
        using Type = Velocity;
    };
}
