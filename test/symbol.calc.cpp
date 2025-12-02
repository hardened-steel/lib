#include <lib/static.string.hpp>
#include <lib/typetraits/list.hpp>
#include <gtest/gtest.h>
#include <string_view>
#include <type_traits>

class Symbol
{
};

template <auto K>
struct Coefficient
{
    constexpr static inline auto value = K;
};

template <class ...Adds>
struct Add
{
    using Type = lib::typetraits::List<Adds...>;
};

template <class ...Muls>
struct Mul
{
    using Type = lib::typetraits::List<Muls...>;
};

TEST(lib, symbols)
{

}
