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
            typename Function::OType result;

        public:
            template <std::size_t ...I>
            FunctionResult(const Function& function, const std::tuple<Wrapper<TArgs>...>& args, std::index_sequence<I...>)
                noexcept(std::is_nothrow_move_constructible_v<Function>)
            : result(function(std::get<I>(args)...))
            {}

            FunctionResult(const Function& function, const std::tuple<Wrapper<TArgs>...>& args) noexcept(std::is_nothrow_move_constructible_v<Function>)
            : FunctionResult(function, args, std::make_index_sequence<sizeof...(TArgs)>{})
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

        public:
            using OType = std::invoke_result_t<Rhs, std::invoke_result_t<Lhs, Wrapper<LArgs>...>, Wrapper<RArgs>...>;

            OType operator()(Wrapper<LArgs>&& ...l_args, Wrapper<RArgs>&& ...r_args) const
            {
                return rhs(lhs(std::move(l_args)...), std::move(r_args)...);
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
}
