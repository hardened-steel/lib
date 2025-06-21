#include <gtest/gtest.h>
#define LIB_ENABLE_UNIT_TEST
#include <lib/test.hpp>
#include <lib/fp.hpp>
#include <iostream>

unittest {
    std::cout << "test test 2" << std::endl;
    check(false);
};


TEST(lib, all)
{
    ASSERT_TRUE(lib::Test::Run());
}

TEST(FP, start)
{
    lib::fp::Fn<int> function = [] { return 42; };
    ASSERT_EQ(function(), 42);
    function = 13;
    ASSERT_EQ(function(), 13);
}
