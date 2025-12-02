#pragma once
#include <lib/test.hpp>
#include <lib/fp/function.hpp>


namespace lib::fp {
    struct FMap
    {
        template <class ...TArgs>
        auto operator() (TArgs&& ...args) const
        {
            return fmap_overload(std::forward<TArgs>(args)...);
        }
    };

    namespace details {
        template <class A, class B, Signature<A, B> ABSignature, template <class> class T, class ABImpl, Signature<T<A>> VSignature, class VImpl>
        struct FnTypeF<typetraits::List<any>, ImplFCall<FMap, Fn<ABSignature, ABImpl>, Fn<VSignature, VImpl>>>
        {
            using Type = Fn<signature<T<B>>, ImplFCall<FMap, Fn<ABSignature, ABImpl>, Fn<VSignature, VImpl>>>;
        };

        template <class A, class B, class C, Signature<A, B> ABSignature, Signature<B, C> BCSignature, class BCImpl, class ABImpl>
        struct FnTypeF<typetraits::List<any>, ImplFCall<FMap, Fn<BCSignature, BCImpl>, Fn<ABSignature, ABImpl>>>
        {
            using Type = Fn<signature<A, C>, ImplFCall<FMap, Fn<BCSignature, BCImpl>, Fn<ABSignature, ABImpl>>>;
        };

        template <class A, class B, class C, class V, Signature<A, B> ABSignature, Signature<B, C> BCSignature, Signature<V> VSignature, class BCImpl, class ABImpl, class VImpl>
        struct FnTypeF<typetraits::List<any>, ImplFCall<FMap, Fn<BCSignature, BCImpl>, Fn<ABSignature, ABImpl>, Fn<VSignature, VImpl>>>
        {
            using BType = std::invoke_result_t<Fn<ABSignature, ABImpl>, Fn<VSignature, VImpl>>;
            using CType = std::invoke_result_t<Fn<BCSignature, BCImpl>, BType>;
            using Type = Fn<signature<typename CType::Result>, ImplFCall<FMap, Fn<BCSignature, BCImpl>, Fn<ABSignature, ABImpl>, Fn<VSignature, VImpl>>>;
        };
    }

    const auto fmap = Fn<signature<any, any, any>, ImplFCall<FMap>>(FMap{});

    template <class A, class B, class C, class BCImpl, class ABImpl, class Value, Signature<A, B> ABSignature, Signature<B, C> BCSignature>
    auto fmap_overload(Fn<BCSignature, BCImpl> lhs, Fn<ABSignature, ABImpl> rhs, Value&& value)
    {
        return lhs(rhs(std::forward<Value>(value)));
    }

    struct FInc
    {
        template <auto Sign, class Impl>
        Fn<Sign, Dynamic> operator() (Fn<Sign, Impl> value) const
        {
            co_return co_await value + 1;
        }
    };

    namespace details {
        template <class A, Signature<A> ASignature, class AImpl>
        struct FnTypeF<typetraits::List<any>, ImplFCall<FInc, Fn<ASignature, AImpl>>>
        {
            using Type = Fn<signature<A>, ImplFCall<FInc, Fn<ASignature, AImpl>>>;
        };
    }

    unittest {
        const auto inc = Fn<signature<any, any>, ImplFCall<FInc>>(FInc{});
        auto d_inc = fmap(inc, inc);

        SimpleExecutor executor;
        check(d_inc(1)(executor) == 3);
    }
}
