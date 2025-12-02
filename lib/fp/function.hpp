#pragma once
#include <lib/test.hpp>
#include <lib/fp/impl.coroutine.hpp>
#include <lib/fp/impl.function.hpp>
#include <lib/fp/impl.value.hpp>


namespace lib::fp {
    /*template <StaticString Signature, class Impl>
    class Fn<Signature, Impl>: public Fn<details::ParseFunction<typetraits::VTag<Signature>>::signature, Impl>
    {
        using Base = Fn<details::ParseFunction<typetraits::VTag<Signature>>::signature, Impl>;

    public:
        using Base::Base;
        using Base::operator();
    };

    namespace details {
        template <class Function>
        class FunctionWrapper
        {
            Function function;

        public:
            FunctionWrapper(Function function) noexcept
            : function(std::move(function))
            {}

            template <auto ...Signatures, class ...TArgs>
            auto operator() (Fn<Signatures, TArgs> ...args) const
            {
                return function(std::move(args)...);
            }
        };
    }

    template <StaticString Signature, class Function>
    auto fn(Function&& function)
    {
        return Fn<
            details::ParseFunction<typetraits::VTag<Signature>>::signature,
            ImplFCall<
                details::FunctionWrapper<std::remove_reference_t<Function>>
            >
        >
        {
            std::forward<Function>(function)
        };
    }

    unittest {
        SimpleExecutor executor;

        const auto inc = fn<"A -> A">(
            []<auto Sign, class Impl>(Fn<Sign, Impl> a) -> Fn<Sign, Dynamic> {
                co_return co_await a + 1;
            }
        );
        check(inc(1)(executor) == 2);
    }*/

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
