#pragma once
#include <lib/fp/function.hpp>
#include <lib/fp/map.hpp>


namespace lib::fp {
    template <class ...Ts>
    using Maybe = Val<std::optional<std::tuple<Ts...>>>;

    template <class ...Ts, class ...To, class Impl>
    Maybe<To...> map(Maybe<Ts...> lhs, Fn<I(Ts...), O(To...), Impl> convertor)
    {
        if (auto value = co_await lhs) {
            co_return co_await convertor(*value);
        }
        co_return std::nullopt;
    }

    unittest {
        SimpleExecutor executor;

        const Fn<I(int), O(int)> half = [](int value) {
            return value / 2;
        };
        check(map(Maybe<int>(1), half)(executor) == 0);
        check((Maybe<int>(2) | half)(executor) == 1);
        check(map(Maybe<int>(std::nullopt), half)(executor) == std::nullopt);
    }
}
