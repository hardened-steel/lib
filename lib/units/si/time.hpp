#pragma once
#include <lib/units.hpp>
#include <chrono>

namespace lib {
    namespace units {
        struct Second
        {
            using Dimension = Second;
            constexpr static auto name() noexcept
            {
                return string("second");
            }
            constexpr static auto symbol() noexcept
            {
                return string("s");
            }
        };
    
        struct Minute
        {
            using Dimension = Minute;
            constexpr static auto name() noexcept
            {
                return string("minute");
            }
            constexpr static auto symbol() noexcept
            {
                return string("m");
            }
        };

        template<>
        struct Convert<Minute, Second>
        {
            using Coefficient = std::ratio<60>;
        };
    
        struct Hour
        {
            using Dimension = Hour;
            constexpr static auto name() noexcept
            {
                return string("hour");
            }
            constexpr static auto symbol() noexcept
            {
                return string("h");
            }
        };

        template<>
        struct Convert<Hour, Minute>
        {
            using Coefficient = std::ratio<60>;
        };

        template<>
        struct Convert<Hour, Second>
        {
            using Coefficient = std::ratio<60 * 60>;
        };

        using namespace std::chrono_literals;
    }

    template<class T, class Ratio>
    class Quantity<units::Second, T, Ratio>: public QuantityBase<units::Second, T, Ratio>
    {
        using Base = QuantityBase<units::Second, T, Ratio>;
    public:
        using Base::Base;
        using Base::operator=;
        
        template<class IRatio>
        constexpr Quantity(std::chrono::duration<T, IRatio> duration) noexcept
        : Quantity(Quantity<units::Second, T, IRatio>(duration.count()))
        {}

        template<class IRatio>
        Quantity& operator=(std::chrono::duration<T, IRatio> duration) noexcept
        {
            Base::quantity = Quantity<units::Second, T, IRatio>(duration.count());
            return *this;
        }
    public:
        constexpr operator std::chrono::duration<T, std::ratio<1>>() const noexcept
        {
            return {Quantity<units::Second, T>(*this).count()};
        }
    };

    template<class T>
    Quantity(std::chrono::duration<T, std::ratio<1>>) -> Quantity<units::Second, T, std::ratio<1>>;

    template<class T, class Ratio>
    class Quantity<units::Minute, T, Ratio>: public QuantityBase<units::Minute, T, Ratio>
    {
        using Base = QuantityBase<units::Minute, T, Ratio>;
    public:
        using Base::Base;
        using Base::operator=;

        template<class IRatio>
        constexpr Quantity(std::chrono::duration<T, IRatio> duration) noexcept
        : Quantity(Quantity<units::Minute, T, IRatio>(duration.count()))
        {}

        template<class IRatio>
        Quantity& operator=(std::chrono::duration<T, IRatio> duration) noexcept
        {
            Base::quantity = Quantity<units::Minute, T, IRatio>(duration.count());
            return *this;
        }
    public:
        constexpr operator std::chrono::duration<T, std::chrono::minutes>() const noexcept
        {
            return {Quantity<units::Minute, T>(*this).count()};
        }
    };

    template<class T>
    Quantity(std::chrono::duration<T, std::ratio<60>>) -> Quantity<units::Minute, T>;

    template<class T, class Ratio>
    class Quantity<units::Hour, T, Ratio>: public QuantityBase<units::Hour, T, Ratio>
    {
        using Base = QuantityBase<units::Hour, T, Ratio>;
    public:
        using Base::Base;
        using Base::operator=;

        template<class IRatio>
        constexpr Quantity(std::chrono::duration<T, IRatio> duration) noexcept
        : Quantity(Quantity<units::Hour, T, IRatio>(duration.count()))
        {}

        template<class IRatio>
        Quantity& operator=(std::chrono::duration<T, IRatio> duration) noexcept
        {
            Base::quantity = Quantity<units::Hour, T, IRatio>(duration.count());
            return *this;
        }
    public:
        constexpr operator std::chrono::duration<T, std::chrono::minutes>() const noexcept
        {
            return {Quantity<units::Hour, T>(*this).count()};
        }
    };

    template<class T>
    Quantity(std::chrono::duration<T, std::ratio<60 * 60>>) -> Quantity<units::Hour, T>;

    template<class T, class Ratio>
    Quantity(std::chrono::duration<T, Ratio>) -> Quantity<units::Second, T, Ratio>;

    template<class Unit, class A, class RatioA, class B, class RatioB>
    constexpr auto operator* (const Quantity<Unit, A, RatioA>& a, std::chrono::duration<B, RatioB> b) noexcept
    {
        return a * Quantity(b);
    }

    template<class Unit, class A, class RatioA, class B, class RatioB>
    constexpr auto operator* (std::chrono::duration<A, RatioA> a, const Quantity<Unit, B, RatioB>& b) noexcept
    {
        return Quantity(a) * b;
    }

    template<class Unit, class A, class RatioA, class B, class RatioB>
    constexpr auto operator/ (const Quantity<Unit, A, RatioA>& a, std::chrono::duration<B, RatioB> b) noexcept
    {
        return a / Quantity(b);
    }

    template<class Unit, class A, class RatioA, class B, class RatioB>
    constexpr auto operator/ (std::chrono::duration<A, RatioA> a, const Quantity<Unit, B, RatioB>& b) noexcept
    {
        return Quantity(a) / b;
    }

    namespace units {
        template<char ...Chars>
        constexpr auto operator ""_s() noexcept
        {
            using Parser = literal::Parser<Chars...>;
            return Quantity<units::Second, typename Parser::Type>(Parser::value);
        }

        inline constexpr auto operator ""_s(long double quantity) noexcept
        {
            return Quantity<units::Second, long double>(quantity);
        }

        template<char ...Chars>
        constexpr auto operator ""_min() noexcept
        {
            using Parser = literal::Parser<Chars...>;
            return Quantity<units::Minute, typename Parser::Type>(Parser::value);
        }

        inline constexpr auto operator ""_min(long double quantity) noexcept
        {
            return Quantity<units::Minute, long double>(quantity);
        }

        template<char ...Chars>
        constexpr auto operator ""_h() noexcept
        {
            using Parser = literal::Parser<Chars...>;
            return Quantity<units::Hour, typename Parser::Type>(Parser::value);
        }

        inline constexpr auto operator ""_h(long double quantity) noexcept
        {
            return Quantity<units::Hour, long double>(quantity);
        }

        template<char ...Chars>
        constexpr auto operator ""_ms() noexcept
        {
            using Parser = literal::Parser<Chars...>;
            return Quantity<units::Second, typename Parser::Type, std::milli>(Parser::value);
        }

        inline constexpr auto operator ""_ms(long double quantity) noexcept
        {
            return Quantity<units::Second, long double, std::milli>(quantity);
        }

        template<char ...Chars>
        constexpr auto operator ""_us() noexcept
        {
            using Parser = literal::Parser<Chars...>;
            return Quantity<units::Second, typename Parser::Type, std::micro>(Parser::value);
        }

        inline constexpr auto operator ""_us(long double quantity) noexcept
        {
            return Quantity<units::Second, long double, std::micro>(quantity);
        }

        template<char ...Chars>
        constexpr auto operator ""_ns() noexcept
        {
            using Parser = literal::Parser<Chars...>;
            return Quantity<units::Second, typename Parser::Type, std::nano>(Parser::value);
        }

        inline constexpr auto operator ""_ns(long double quantity) noexcept
        {
            return Quantity<units::Second, long double, std::nano>(quantity);
        }
    }
}
