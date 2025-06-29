#pragma once
#include <lib/fp/function.hpp>


namespace lib::fp {

    template <class ...IParams, class Middle, class To>
    Fn<To(IParams...)> map(Fn<Middle(IParams...)> lhs, Fn<To(Middle)> rhs)
    {
        return rhs(lhs);
    }

    template <class ...IParams, class Middle, class To>
    auto operator | (Fn<Middle(IParams...)> lhs, Fn<To(Middle)> rhs)
    {
        return map(lhs, rhs);
    }

    template <class T, class From, class To>
    Val<To> map(T value, Fn<To(From)> convertor)
    {
        return map(Fn(value), convertor);
    }

    template <class T, class From, class To>
    auto operator | (T value, Fn<To(From)> convertor)
    {
        return map(value, convertor);
    }

    unittest {
        SimpleWorker worker;

        const Fn<std::string(int)> convertor = [](int value) {
            return std::to_string(value);
        };
        check(map(0, convertor)(worker) == "0");
        check(map(1, convertor)(worker) == "1");
        check(map(3, convertor)(worker) == "3");

        const Val<int> value = 42;
        check(map(value, convertor)(worker) == "42");
    }
}
