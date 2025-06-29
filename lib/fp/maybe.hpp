#pragma once
#include <lib/fp/function.hpp>
#include <lib/fp/map.hpp>


namespace lib::fp {
    template <class T>
    using Maybe = Val<std::optional<T>>;

    template <class From, class To>
    Maybe<To> map(Maybe<From> lhs, Fn<To(From)> rhs)
    {
        if (auto value = co_await lhs) {
            co_return co_await rhs(*value);
        }
        co_return std::nullopt;
    }

    template <class From, class To>
    auto operator | (Maybe<From> lhs, Fn<To(From)> rhs)
    {
        return map(lhs, rhs);
    }

    template <class R, class Lhs, class Rhs>
    Maybe<R> apply(Maybe<Lhs> lhs, Maybe<Rhs> rhs, Fn<R(Lhs, Rhs)> function)
    {
        const auto& l_item = co_await lhs;
        const auto& r_item = co_await rhs;

        if (l_item && r_item) {
            co_return co_await function(l_item, r_item);
        }
        co_return std::nullopt;
    }

    unittest {
        SimpleWorker worker;

        const Fn<int(int)> half = [](int value) {
            return value / 2;
        };
        check(map(Maybe<int>(1), half)(worker) == 0);
        check((Maybe<int>(2) | half)(worker) == 1);
        check(map(Maybe<int>(std::nullopt), half)(worker) == std::nullopt);
    }
}
