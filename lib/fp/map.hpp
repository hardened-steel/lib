#pragma once
#include <lib/fp/function.hpp>


namespace lib::fp {

    /*template <class From, class To>
    Fn<To> map(Fn<From> value, Fn<To(From)> converter)
    {
        co_return co_await converter(co_await value);
    }*/

    /*template <class ...IParams, class Middle, class To>
    Fn<To(IParams...)> map(Fn<Middle(IParams...)> lhs, Fn<To(Middle)> rhs)
    {
        return [=] (IParams ...args) -> To {
            co_return co_await rhs(lhs(co_await args...));
        };
    }

    unittest {
        const Fn<std::string(int)> convertor = [](int value) {
            return std::to_string(value);
        };
        check(map(Fn(0), convertor)() == "0");
        check(map(Fn(1), convertor)() == "1");
        check(map(Fn(42), convertor)() == "42");

        const Fn<int> value = 13;
        check(map(value, convertor)() == "13");
    }*/
}
