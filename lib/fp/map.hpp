#pragma once
#include <lib/fp/function.hpp>


namespace lib::fp {

    template <class ...IParams, class ...MParams, class ...OParams, class LImpl, class RImpl>
    auto map(Fn<I(IParams...), O(MParams...), LImpl> lhs, Fn<I(MParams...), O(OParams...), RImpl> rhs)
    {
        return rhs(lhs);
    }

    template <class Lhs, class Rhs>
    requires (is_function<Lhs> || is_function<Rhs>)
    auto operator | (Lhs lhs, Rhs rhs)
    {
        return map(lhs, rhs);
    }

    template <class From, class To, class Impl>
    Val<To> map(From value, Fn<I(From), O(To), Impl> convertor)
    {
        return map(Fn(value), convertor);
    }

    unittest {
        SimpleExecutor executor;

        const Fn<I(int), O(std::string)> convertor = [](int value) {
            return std::to_string(value);
        };
        check(map(0, convertor)(executor) == "0");
        check(map(1, convertor)(executor) == "1");
        check(map(3, convertor)(executor) == "3");
        check((4 | convertor)(executor) == "4");

        const Fn value = 42;
        check(map(value, convertor)(executor) == "42");
    }
}
