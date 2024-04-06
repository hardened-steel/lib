#pragma once
#include <lib/units.hpp>
#include <lib/units/si/length.hpp>
#include <lib/quantity.hpp>

namespace lib::units {
    template<class Length>
    struct Velocity
    {
        using Dimension = Divide<Length, Second>;
        constexpr static auto name() noexcept
        {
            return string("velocity");
        }
        constexpr static auto symbol() noexcept
        {
            return Length::symbol() + string("/s");
        }
    };

    template<class Length>
    struct Dimension<Multiplying<Length, TDegree<Second, -1>>>
    {
        using Type = Velocity<Length>;
    };

}