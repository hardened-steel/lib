#include <lib/units.hpp>
#include <lib/units/electrical.resistance.hpp>
#include <lib/units/voltage.hpp>
#include <gtest/gtest.h>
#include <lib/quantity.hpp>
#include <iostream>

struct A {
    using Dimension = A;
};
struct B {
    using Dimension = B;
};

TEST(lib, units)
{
    const lib::Quantity<A, int> a {100};
    const lib::Quantity<B, int> b {10};
    const auto c = a / b;
    const auto c1 = c / a;
    std::cout << lib::type_name<decltype(c / a)> << std::endl;
    std::cout << lib::type_name<decltype(1 / b)> << std::endl;
    std::cout << lib::type_name<decltype(c * b)> << std::endl;
    std::cout << lib::type_name<decltype(c / a * b)> << std::endl;

    using namespace lib::units;
    const auto R = 100_ohm;
    //kg⋅m2⋅s−3⋅A−2
    const auto R2 = (1_kg * 1_m) * (1_m / lib::Quantity<Time, int>(1)) * (1 / (lib::Quantity<Time, int>(1) * (lib::Quantity<Time, int>(1) * 1_A) * 1_A));
    const auto I1 = 10_V / R;
    //EXPECT_EQ(c / a, 1 / b);
    //Dimension
}