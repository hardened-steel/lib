#include <gtest/gtest.h>

#include <lib/typetraits/list.hpp>
#include <lib/typetraits/for.hpp>
#include <lib/typetraits/set.hpp>
#include <lib/typetraits/map.hpp>
#include <lib/typetraits/interpreter.hpp>
#include <type_traits>



using namespace lib::typetraits;



TEST(typetraits, Interpreter)
{
    using namespace lib::typetraits::interpreter;
    static_assert(Call<Factorial<Value<5>>>::value == 120);
    static_assert(Call<Factorial<Value<4>>>::value == 24);
    static_assert(Call<Factorial<Value<0>>>::value == 1);
    EXPECT_EQ(Call<ErrorFunction>::message, "write to undefined variable 'ErrorFunction::Variable'");
    static_assert(Call<BarFunction<Value<10>>>::value == 52);
    static_assert(lib::typetraits::interpreter::impl::IsFunction<BarFunction<Value<10>>>);
}

