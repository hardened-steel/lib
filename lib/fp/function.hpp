#pragma once
#include <lib/fp/impl.coroutine.hpp>
#include <lib/fp/impl.function.hpp>
#include <lib/fp/impl.value.hpp>


namespace lib::fp {
    template <StaticString Signature, class Impl>
    class Fn<Signature, Impl>: public Fn<details::ParseFunction<typetraits::VTag<Signature>>::signature, Impl>
    {
        using Base = Fn<details::ParseFunction<typetraits::VTag<Signature>>::signature, Impl>;

    public:
        using Base::Base;
        using Base::operator();
    };

    template <StaticString Signature, class Function>
    auto fn(Function&& function)
    {
        return Fn<Signature, ImplFCall<std::remove_cvref_t<Function>>>(std::forward<Function>(function));
    }

    unittest {
        SimpleExecutor executor;

        const auto function = fn<"A -> A">([](auto a) { co_return co_await a; });
        //check(function(1)(executor) == 1);
        //check(function("hello")(executor) == "hello");
    }

    unittest {
        SimpleExecutor executor;

        const auto sum_coro = [](Val<int> lhs, Val<int> rhs) -> Val<int> {
            co_return co_await lhs + co_await rhs;
        };

        Fn<signature<int, int, int>, ImplFCall<decltype(sum_coro)>> sum = sum_coro;
        check(sum(0, details::test_coro())(executor) == 42);
        check(sum(1, 2)(executor) == 3);
        check(sum(3)(2)(executor) == 5);
    }

    unittest {
        SimpleExecutor executor;

        int (*inc_fn)(int) = [] (int value) noexcept {
            return value + 1;
        };
        Fn inc = inc_fn;

        const auto dec = [](Val<int> value) -> Val<int> {
            co_return co_await value - 1;
        };
        check(dec(inc(0))(executor) == 0);
    }

    unittest {
        SimpleExecutor executor;

        std::string (*do_concat)(std::string_view lhs, const char* rhs) = [](std::string_view lhs, const char* rhs) -> std::string {
            std::string result(lhs.begin(), lhs.end());
            result.append(rhs);
            return result;
        };
        Fn concat = do_concat;
        check(concat(std::string_view("hello"), "world")(executor) == "helloworld");
        check(concat(std::string_view("hello"))("world")(executor) == "helloworld");
    }
}
