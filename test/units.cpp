#include <iterator>
#include <lib/static.string.hpp>
#include <lib/units.hpp>
#include <lib/units/electrical.resistance.hpp>
#include <lib/units/voltage.hpp>
#include <lib/units/pascal.hpp>
#include <lib/units/si/time.hpp>
#include <lib/units/velocity.hpp>
#include <lib/units.io.hpp>
#include <gtest/gtest.h>
#include <lib/quantity.hpp>
#include <string_view>
#include <type_traits>

struct A {
    using Dimension = A;
    constexpr static auto symbol() noexcept
    {
        return lib::StaticString("A");
    }
};
struct B {
    using Dimension = B;
    constexpr static auto symbol() noexcept
    {
        return lib::StaticString("B");
    }
};

namespace lib::units {
    struct Turn
    {
        using Dimension = Turn;
        constexpr static auto name() noexcept
        {
            return string("turns");
        }
        constexpr static auto symbol() noexcept
        {
            return string("turn");
        }
    };
    constexpr inline Unit<units::Turn> turn {};

    template<char ...Chars>
    constexpr auto operator "" _turn() noexcept
    {
        using Parser = literal::Parser<Chars...>;
        return Quantity<Turn, typename Parser::Type>(Parser::value);
    }

    inline constexpr auto operator "" _turn(long double quantity) noexcept
    {
        return Quantity<Turn, long double>(quantity);
    }

    struct Rpm
    {
        using Dimension = Divide<Turn, Minute>;
        constexpr static auto name() noexcept
        {
            return string("revolutions per minute");
        }
        constexpr static auto symbol() noexcept
        {
            return string("rpm");
        }
    };
    template<>
    struct Dimension<Rpm::Dimension>
    {
        using Type = Rpm;
    };
    constexpr inline Unit<Rpm> rpm {};

    template<char ...Chars>
    constexpr auto operator "" _rpm() noexcept
    {
        using Parser = literal::Parser<Chars...>;
        return Quantity<Rpm, typename Parser::Type>(Parser::value);
    }

    inline constexpr auto operator "" _rpm(long double quantity) noexcept
    {
        return Quantity<Rpm, long double>(quantity);
    }

    struct Mile
    {
        using Dimension = Mile;
        constexpr static auto name() noexcept
        {
            return string("Mile");
        }
        constexpr static auto symbol() noexcept
        {
            return string("mile");
        }
    };

    template<>
    struct Convert<Mile, Metre>
    {
        using Coefficient = std::ratio<1609344, 1000>;
    };

    template<char ...Chars>
    constexpr auto operator "" _mile() noexcept
    {
        using Parser = literal::Parser<Chars...>;
        return Quantity<Mile, typename Parser::Type>(Parser::value);
    }

    inline constexpr auto operator "" _mile(long double quantity) noexcept
    {
        return Quantity<Mile, long double>(quantity);
    }
}

TEST(lib, units)
{
    using namespace std::chrono_literals;
    using namespace lib::units;
    {
        const lib::Quantity<A, float> a {100};
        const lib::Quantity<B, int> b {10};
        const auto c = a / b;
        const auto l = 1 / c;
        const auto r = c / a;

        EXPECT_FLOAT_EQ(l.count(), r.count());
    }
    {
        const auto R = 100_ohm;
        // kg⋅m2⋅s−3⋅A−2
        const auto R2 = (1_kg * 1_m) * (1_m / lib::Quantity<Second, int>(1)) * (1 / (lib::Quantity<Second, int>(1) * (lib::Quantity<Second, int>(1) * 1_A) * 1_A));
        EXPECT_EQ(R2 * 100, R);
        const auto A = 2_m * 3_m;
        const auto P = 12_N / A;
        EXPECT_EQ(2_Pa, P);
    }
    {
        lib::Quantity count = 100;
        EXPECT_EQ(count.count(), 100);

        const auto time1 = 1_s;
        lib::Quantity time2 = time1;
        EXPECT_EQ(time2.count(), time1.count());
    }
    {
        EXPECT_EQ(1s / 100ms, 10);
        lib::Quantity time1 = 1s;
        lib::Quantity time2 = 100ms;
        lib::Quantity count = 10;
        EXPECT_EQ(time1 / time2, count);
        EXPECT_EQ(time2 * 10, time1);
    }
    {
        auto fanspeed = 100_turn / 1_s;
        EXPECT_EQ(fanspeed, 6000_rpm);
    }
    {
        auto fanspeed = 1_turn / 1_min;
        EXPECT_EQ(fanspeed, 1_rpm);
    }
    {
        auto fanspeed = 2_turn / 1_min;
        EXPECT_EQ(fanspeed, 2_rpm);
    }
    {
        auto fanspeed = 60_turn / 1_h;
        EXPECT_EQ(fanspeed, 1_rpm);
    }
    {
        auto fanspeed = 100_turn / 2_min;
        auto half = 100_rpm / 2;
        EXPECT_EQ(fanspeed, half);
        EXPECT_EQ(fanspeed.count(), 50);
    }
    {
        auto fanspeed = 100_turn / 2_min;
        auto turns = fanspeed * 6_s;
        EXPECT_EQ(turns, 5_turn);
    }
    {
        lib::Quantity speed1 = 100.0_mile / 1_h;
        //lib::Quantity<lib::units::Velocity<Metre>, float> speed2 = speed1;
        std::cout << lib::type_name<decltype(speed1)> << ": " << speed1 << std::endl;
        //std::cout << lib::type_name<decltype(speed2)> << ": " << speed2 << std::endl;
    }
}
