#include <iterator>
#include <lib/static.string.hpp>
#include <lib/units.hpp>
#include <lib/units/electrical.resistance.hpp>
#include <lib/units/voltage.hpp>
#include <lib/units/pascal.hpp>
#include <lib/units.io.hpp>
#include <gtest/gtest.h>
#include <lib/quantity.hpp>
#include <iostream>
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

TEST(lib, units)
{
    const lib::Quantity<A, float> a {100};
    const lib::Quantity<B, int> b {10};
    const auto c = a / b;
    const auto c1 = c / a;

    EXPECT_FLOAT_EQ((1 / c).count(), (c / a).count());

    using namespace lib::units;
    const auto R = 100_ohm;
    //kg⋅m2⋅s−3⋅A−2
    const auto R2 = (1_kg * 1_m) * (1_m / lib::Quantity<Time, int>(1)) * (1 / (lib::Quantity<Time, int>(1) * (lib::Quantity<Time, int>(1) * 1_A) * 1_A));
    EXPECT_EQ(R2 * 100, R);
    const auto A = 2_m * 3_m;
    const auto P = 12_N / A;
    EXPECT_EQ(2_Pa, P);
}