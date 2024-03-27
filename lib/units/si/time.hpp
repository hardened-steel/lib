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

    template<class Unit, class A, class RatioA, class B, class RatioB>
    constexpr auto operator* (const Quantity<Unit, A, RatioA>& a, std::chrono::duration<B, RatioB> b) noexcept
    {
        return a * Quantity<units::Time, B, RatioB>(b);
    }

    template<class Unit, class A, class RatioA, class B, class RatioB>
    constexpr auto operator* (std::chrono::duration<A, RatioA> a, const Quantity<Unit, B, RatioB>& b) noexcept
    {
        return Quantity<units::Time, A, RatioA>(a) * b;
    }

    template<class Unit, class A, class RatioA, class B, class RatioB>
    constexpr auto operator/ (const Quantity<Unit, A, RatioA>& a, std::chrono::duration<B, RatioB> b) noexcept
    {
        return a / Quantity<units::Time, B, RatioB>(b);
    }

    template<class Unit, class A, class RatioA, class B, class RatioB>
    constexpr auto operator/ (std::chrono::duration<A, RatioA> a, const Quantity<Unit, B, RatioB>& b) noexcept
    {
        return Quantity<units::Time, A, RatioA>(a) / b;
    }

    namespace units {
        template<char ...Chars>
        constexpr auto operator ""_s() noexcept
        {
            using Parser = literal::Parser<Chars...>;
            return Quantity<units::Time, typename Parser::Type>(Parser::value);
        }

        inline constexpr auto operator ""_s(long double quantity) noexcept
        {
            return Quantity<units::Time, long double>(quantity);
        }

        template<char ...Chars>
        constexpr auto operator ""_min() noexcept
        {
            using Parser = literal::Parser<Chars...>;
            return Quantity<units::Time, typename Parser::Type, std::ratio<60>>(Parser::value);
        }

        inline constexpr auto operator ""_min(long double quantity) noexcept
        {
            return Quantity<units::Time, long double, std::ratio<60>>(quantity);
        }

        template<char ...Chars>
        constexpr auto operator ""_h() noexcept
        {
            using Parser = literal::Parser<Chars...>;
            return Quantity<units::Time, typename Parser::Type, std::ratio<60 * 60>>(Parser::value);
        }

        inline constexpr auto operator ""_h(long double quantity) noexcept
        {
            return Quantity<units::Time, long double, std::ratio<60 * 60>>(quantity);
        }

        template<char ...Chars>
        constexpr auto operator ""_ms() noexcept
        {
            using Parser = literal::Parser<Chars...>;
            return Quantity<units::Time, typename Parser::Type, std::milli>(Parser::value);
        }

        inline constexpr auto operator ""_ms(long double quantity) noexcept
        {
            return Quantity<units::Time, long double, std::milli>(quantity);
        }

        template<char ...Chars>
        constexpr auto operator ""_us() noexcept
        {
            using Parser = literal::Parser<Chars...>;
            return Quantity<units::Time, typename Parser::Type, std::micro>(Parser::value);
        }

        inline constexpr auto operator ""_us(long double quantity) noexcept
        {
            return Quantity<units::Time, long double, std::micro>(quantity);
        }

        template<char ...Chars>
        constexpr auto operator ""_ns() noexcept
        {
            using Parser = literal::Parser<Chars...>;
            return Quantity<units::Time, typename Parser::Type, std::nano>(Parser::value);
        }

        inline constexpr auto operator ""_ns(long double quantity) noexcept
        {
            return Quantity<units::Time, long double, std::nano>(quantity);
        }
    }
}
