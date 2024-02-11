#pragma once
#include <lib/units.hpp>
#include <lib/literal.hpp>

namespace lib {
    namespace units {
        struct Count
        {
            using Dimension = Multiplying<>;
            constexpr static auto name() noexcept
            {
                return StaticString("counts");
            }
            constexpr static auto symbol() noexcept
            {
                return StaticString("");
            }
        };

        template<>
        struct Dimension<Count::Dimension>
        {
            using Type = Count;
        };
    }

    template<class T, class Ratio>
    class Quantity<units::Count, T, Ratio>: public QuantityBase<lib::Unit<units::Count>, T, Ratio>
    {
        using Base = QuantityBase<lib::Unit<units::Count>, T, Ratio>;
    public:
        using Base::Base;
        using Base::operator=;
        
        constexpr Quantity(T quantity) noexcept
        : Base(quantity)
        {}
        Quantity& operator=(T quantity) noexcept
        {
            Base::quantity = quantity;
            return *this;
        }
    public:
        constexpr operator T() const noexcept
        {
            return {Base::quantity};
        }
    };

    template<class T>
    Quantity(T value) -> Quantity<units::Count, T>;

    template<class Unit, class A, class Ratio, class B>
    constexpr auto operator* (const Quantity<Unit, A, Ratio>& a, B b) noexcept
    {
        using Type = std::common_type_t<A, B>;
        using CommonQuantity = Quantity<units::Count, Type>;
        return a * CommonQuantity(b);
    }

    template<class Unit, class A, class Ratio, class B>
    constexpr auto operator* (A a, const Quantity<Unit, B, Ratio>& b) noexcept
    {
        using Type = std::common_type_t<A, B>;
        using CommonQuantity = Quantity<units::Count, Type>;
        return CommonQuantity(a) * b;
    }

    template<class Unit, class A, class Ratio, class B>
    constexpr auto operator/ (const Quantity<Unit, A, Ratio>& a, B b) noexcept
    {
        using Type = std::common_type_t<A, B>;
        using CommonQuantity = Quantity<units::Count, Type>;
        return a / CommonQuantity(b);
    }

    template<class Unit, class A, class Ratio, class B>
    constexpr auto operator/ (A a, const Quantity<Unit, B, Ratio>& b) noexcept
    {
        using Type = std::common_type_t<A, B>;
        using CommonQuantity = Quantity<units::Count, Type>;
        return CommonQuantity(a) / b;
    }
}
