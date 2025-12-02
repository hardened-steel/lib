#pragma once
#include <lib/test.hpp>
#include <lib/static.string.hpp>
#include <lib/typetraits/list.hpp>


namespace lib::fp {

    template <auto Signature, class Implementation>
    class Fn;

    // entities
    template <class ...Ts>
    struct Signature {};

    template <class ...Ts>
    constexpr Signature<Ts...> signature;

    template <StaticString Name, class ...Args>
    struct Type {};

    template <template <class ...> class T>
    struct Template {};

    template <class T>
    struct JustType {};


    // tags
    struct Dynamic;

    template <class T>
    struct ImplValue;

    namespace details {
        template <class T>
        struct FnWrapper
        {
            using Type = Fn<signature<T>, ImplValue<T>>;
        };

        template <auto Signature, class Impl>
        struct FnWrapper<Fn<Signature, Impl>>
        {
            using Type = Fn<Signature, Impl>;
        };

        template <class T>
        using Wrapper = typename FnWrapper<std::remove_cvref_t<T>>::Type;

        template <class T>
        decltype(auto) wrap(T&& value)
        {
            return Wrapper<T>(std::forward<T>(value));
        }

        template <auto Signature, class Impl>
        decltype(auto) wrap(Fn<Signature, Impl>&& value)
        {
            return std::move(value);
        }

        template <auto Signature, class Impl>
        decltype(auto) wrap(const Fn<Signature, Impl>& value)
        {
            return value;
        }
    }

    template <class F, class ...TArgs>
    struct ImplFCall
    {
        using Function = F;
        using PTypes = typetraits::List<TArgs...>;
        using Params = std::tuple<details::Wrapper<TArgs>...>;
    };

    template <class T>
    using Val = Fn<signature<T>, Dynamic>;
}
