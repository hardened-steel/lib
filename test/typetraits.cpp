#include <gtest/gtest.h>

#include <lib/typetraits/list.hpp>
#include <lib/typetraits/for.hpp>
#include <lib/typetraits/set.hpp>
#include <lib/typetraits/map.hpp>
#include <lib/typetraits/interpreter.hpp>
#include <type_traits>

TEST(typetraits, List)
{
    using namespace lib::typetraits;

    using EmptyList = List<>;
    static_assert(EmptyList::size == 0);
    static_assert(std::is_same_v<InsertBack<EmptyList, void>, List<void>>);
    static_assert(std::is_same_v<InsertFront<EmptyList, void>, List<void>>);
    using L1 = List<int>;
    static_assert(L1::size == 1);
    using L2 = InsertBack<L1, void>;
    static_assert(L2::size == 2);
    static_assert(std::is_same_v<InsertBack<L1, void>, List<int, void>>);
    static_assert(std::is_same_v<InsertFront<L1, void>, List<void, int>>);

    static_assert(std::is_same_v<Get<L2, 0>, int>);
    static_assert(std::is_same_v<Get<L2, 1>, void>);

    static_assert(std::is_same_v<L1, Concat<L1, EmptyList>>);
    static_assert(std::is_same_v<L2, Concat<EmptyList, L2>>);
    static_assert(std::is_same_v<List<int, void, float>, Concat<L2, List<float>>>);
    static_assert(std::is_same_v<List<int, int, int>, Concat<L1, L1, L1>>);
    static_assert(std::is_same_v<L1, Concat<L1>>);

    static_assert(std::is_same_v<L1, Remove<L2, 1>>);
    static_assert(std::is_same_v<EmptyList, Remove<L1, 0>>);
}

TEST(typetraits, For)
{
    using namespace lib::typetraits;

    using L = List<int, void, float, double>;

}

TEST(typetraits, Set)
{
    using namespace lib::typetraits;

    using EmptySet = Set<>;
    static_assert(std::is_same_v<Set<int>, Insert<EmptySet, int>>);
    static_assert(std::is_same_v<Set<int>, Insert<Set<int>, int>>);
    static_assert(std::is_same_v<Set<float, int>, Insert<Set<int>, float>>);
    static_assert(std::is_same_v<Set<float, int, void>, Insert<Set<float, int>, void>>);

    using S = Insert<Insert<Insert<EmptySet, void>, int>, float>;
    static_assert(std::is_same_v<Get<S, 0>, float>);
    static_assert(std::is_same_v<Get<S, 1>, int>);
    static_assert(std::is_same_v<Get<S, 2>, void>);

    static_assert(std::is_same_v<Insert<Insert<Insert<S, int>, float>, void>, Insert<Insert<Insert<S, float>, int>, void>>);
    static_assert(lib::typetraits::index<Set<float, int>, int> == 1);
    static_assert(lib::typetraits::index<Set<float, int>, float> == 0);
    static_assert(lib::typetraits::index<S, unsigned> == S::size);

    static_assert(exists<S, float>);
    static_assert(!exists<S, unsigned>);
    static_assert(std::is_same_v<Erase<S, float>, Set<int, void>>);
    static_assert(std::is_same_v<Erase<S, unsigned>, S>);

    static_assert(std::is_same_v<S, Merge<EmptySet, S>>);
    static_assert(std::is_same_v<S, Merge<EmptySet, EmptySet, Insert<Insert<S, float>, int>, Set<void>>>);
    static_assert(std::is_same_v<S, Merge<EmptySet, Insert<Set<float>, int>, EmptySet, Set<void>>>);
}

TEST(typetraits, Map)
{
    using namespace lib::typetraits;

    using EmptyMap = Map<>;
    static_assert(std::is_same_v<Map<Map<int, void>>, Insert<EmptyMap, Map<int, void>>>);
    using TestMap = Insert<Insert<Insert<EmptyMap, Map<int, void>>, Map<int, void>>, Map<float, char>>;
    static_assert(std::is_same_v<Find<TestMap, int>, void>);
    static_assert(std::is_same_v<Find<TestMap, float>, char>);
    static_assert(lib::typetraits::exists<TestMap, int>);
    static_assert(lib::typetraits::index<TestMap, int> == 0);
    static_assert(lib::typetraits::index<TestMap, float> == 1);
    static_assert(lib::typetraits::exists<TestMap, float>);
}

using namespace lib::typetraits;

template <class Param>
struct Factorial: interpreter::Function
{
    struct Result;
    struct Counter;
    using Body = Scope<
        CreateVariable<Result, Value<1>>,
        If<Eq<Param, Value<0>>,
            Return<Result>
        >,
        CreateVariable<Counter, Value<1>>,
        While<Not<Eq<Counter, Param>>,
            Write<Result, Mul<Result, Counter>>,
            Write<Counter, Inc<Counter>>
        >,
        Return<Mul<Result, Counter>>
    >;
};

struct ErrorFunction: interpreter::Function
{
    struct Variable;
    using Body = Scope<
        Write<Variable, Value<42>>,
        Return<Variable>
    >;
};

template <class A, class B>
struct FooFunction: interpreter::Function
{
    using Body = Scope<
        Return<Add<A, B>>
    >;
};

template <class IValue>
struct BarFunction: interpreter::Function
{
    struct Result;
    using Body = Scope<
        CreateVariable<Result, IValue>,
        Return<Add<FooFunction<Result, Value<42>>, Value<0>>>
    >;
};

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

