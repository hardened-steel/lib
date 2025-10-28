#pragma once
#include <lib/fp/base.function.hpp>
#include <memory>


namespace lib::fp {
    namespace details {
        template <class T>
        struct FunctionPtrT
        {
            using Type = std::shared_ptr<T>;
        };

        template <class R, class ...TArgs>
        struct FunctionPtrT<R(*)(TArgs...)>
        {
            using Type = R(*)(TArgs...);
        };

        template <auto Signature, class Impl>
        struct FunctionPtrT<Fn<Signature, Impl>>
        {
            using Type = Fn<Signature, Impl>;
        };
    }

    template <class T>
    using FunctionPtr = typename details::FunctionPtrT<T>::Type;
}
