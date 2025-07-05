#pragma once
#include <lib/fp/function.hpp>
#include <utility>


namespace lib::fp {

    namespace details {
        template <class FLhs, class ALhs, class FRhs, class ARhs>
        class MapFunction;

        template <class FLhs, class ALhs, class FRhs, class ARhs>
        struct FunctionPtrT<MapFunction<FLhs, ALhs, FRhs, ARhs>>
        {
            using Type = MapFunction<FLhs, ALhs, FRhs, ARhs>;
        };

        template <class T>
        using FunctionPtr = typename FunctionPtrT<T>::Type;

        template <class FLhs, class ALhs, class FRhs, class ARhs>
        auto& CreateFunctionPtr(const MapFunction<FLhs, ALhs, FRhs, ARhs>& function) noexcept
        {
            return function;
        }

        template <class FLhs, class ALhs, class FRhs, class ARhs, class R, class ...TArgs>
        class FunctionResult<MapFunction<FLhs, ALhs, FRhs, ARhs>, R, TArgs...>
        {
            using Function = MapFunction<FLhs, ALhs, FRhs, ARhs>;
            std::invoke_result_t<MapFunction<FLhs, ALhs, FRhs, ARhs>, std::tuple<TArgs...>> result;

        public:
            FunctionResult(const Function& function, const std::tuple<TArgs...>& args)
                noexcept(std::is_nothrow_move_constructible_v<Function>)
            : result(function(args))
            {}

        public:
            [[nodiscard]] bool ready() const noexcept
            {
                return result.ready();
            }

            [[nodiscard]] const R& get() const noexcept
            {
                return result.get();
            }

            bool subscribe(IExecutor& executor, IAwaiter& awaiter)
            {
                return result.subscribe(executor, awaiter);
            }

            const R& wait(IExecutor& executor)
            {
                return result(executor);
            }
        };

        template <class Lhs, class Rhs, class ...LArgs, class ...RArgs>
        class MapFunction<Lhs, typetraits::List<LArgs...>, Rhs, typetraits::List<RArgs...>>
        {
            Lhs lhs;
            Rhs rhs;

        public:
            MapFunction(Lhs lhs, Rhs rhs) noexcept
            : lhs(std::move(lhs))
            , rhs(std::move(rhs))
            {}

            MapFunction(const MapFunction& other) noexcept = default;

        private:
            template <class ...TArgs, std::size_t ...LI, std::size_t ...RI>
            [[nodiscard]] auto call(const std::tuple<TArgs...>& args, std::index_sequence<LI...>, std::index_sequence<RI...>) const
            {
                return rhs(lhs(std::get<LI>(args)...), std::get<RI>(args)...);
            }

        public:
            template <class ...TArgs>
            requires (sizeof...(TArgs) == sizeof...(LArgs) + sizeof...(RArgs) && (is_function<TArgs> && ...))
            auto operator()(const std::tuple<TArgs...>& args) const
            {
                return call(
                    args,
                    std::make_index_sequence<sizeof...(LArgs)>{},
                    indexes::Add<sizeof...(LArgs), std::make_index_sequence<sizeof...(RArgs)>>{}
                );
            }
        };
    }


    template <class ...LTs, class ...RTs>
    requires (std::is_convertible_v<typename Fn<LTs...>::Result, typename Fn<RTs...>::IParam>)
    auto map(Fn<LTs...> lhs, Fn<RTs...> rhs)
    {
        using Signature = typetraits::Concat<
            typename Fn<LTs...>::ITypes,
            typetraits::PopFront<typename Fn<RTs...>::ITypes>,
            typetraits::List<typename Fn<RTs...>::Result>
        >;
        using MapFunction = details::MapFunction<
            Fn<LTs...>, typename Fn<LTs...>::ITypes,
            Fn<RTs...>, typetraits::PopFront<typename Fn<RTs...>::ITypes>
        >;
        return FnType<Signature, ImplFCall<MapFunction>>(MapFunction(lhs, rhs));
    }

    template <class T, class F>
    requires (!is_function<std::remove_cvref_t<T>> && is_function<std::remove_cvref_t<F>>)
    auto map(T&& value, F function)
    {
        return map(wrap(std::forward<T>(value)), function);
    }

    template <class F, class T>
    requires (is_function<std::remove_cvref_t<F>> && !is_function<std::remove_cvref_t<T>>)
    auto map(F lhs, T&& rhs)
    {
        return map(lhs, Fn(std::forward<T>(rhs)));
    }

    template <class Lhs, class Rhs>
    requires (is_function<Lhs> || is_function<Rhs>)
    auto operator | (Lhs lhs, Rhs rhs)
    {
        return map(lhs, rhs);
    }

    unittest {
        SimpleExecutor executor;

        std::string(*do_convert)(int) = [](int value) {
            return std::to_string(value);
        };
        Fn convertor = do_convert;
        check(map(0, convertor)(executor) == "0");
        check(map(1, convertor)(executor) == "1");
        check(map(3, convertor)(executor) == "3");
        check((4 | convertor)(executor) == "4");

        const Fn value = 42;
        check(map(value, convertor)(executor) == "42");
    }

    unittest {
        SimpleExecutor executor;

        int (*do_inc)(int) = [](int value) {
            return value + 1;
        };
        const Fn inc = do_inc;

        check((map(1, map(inc, inc)))(executor) == 3);
        check((map(map(1, inc), inc))(executor) == 3);
        check((1 | inc | inc | do_inc)(executor) == 4);
    }
}
