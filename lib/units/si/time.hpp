#pragma once
#include <lib/units.hpp>
#include <chrono>

namespace lib {
    namespace units {
        struct Time
        {
            using Dimension = Time;
            constexpr static auto name() noexcept
            {
                return StaticString("seconds");
            }
            constexpr static auto symbol() noexcept
            {
                return StaticString("s");
            }
        };
        using namespace std::chrono_literals;
    }

    template<class T, class Ratio>
    class Quantity<units::Time, T, Ratio>: public QuantityBase<units::Time, T, Ratio>
    {
        using Base = QuantityBase<units::Time, T, Ratio>;
    public:
        using Base::Base;
        using Base::operator=;
        
        constexpr Quantity(std::chrono::duration<T, Ratio> duration) noexcept
        : Base(duration.count())
        {}
        Quantity& operator=(std::chrono::duration<T, Ratio> duration) noexcept
        {
            Base::quantity = duration.count();
            return *this;
        }
    public:
        constexpr operator std::chrono::duration<T, Ratio>() const noexcept
        {
            return {Base::quantity};
        }
    };

    template<class T, class Ratio>
    Quantity(std::chrono::duration<T, Ratio>) -> Quantity<units::Time, T, Ratio>;
}