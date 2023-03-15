#pragma once
#include <type_traits>
#include <lib/typetraits/list.hpp>

namespace lib::typetraits {

    namespace impl {
    
        template<class Value>
        using Constant = Value;

        template<bool Condition, template<class...> class Then, class TParams, template<class...> class Else, class EParams>
        struct IfF;

        template<bool Condition, template<class...> class Then, class TParams, template<class...> class Else, class EParams>
        using If = typename IfF<Condition, Then, TParams, Else, EParams>::Result;

        template<template<class...> class Then, class ...TParams, template<class...> class Else, class ...EParams>
        struct IfF<true, Then, List<TParams...>, Else, List<EParams...>>
        {
            using Result = Then<TParams...>;
        };

        template<template<class...> class Then, class ...TParams, template<class...> class Else, class ...EParams>
        struct IfF<false, Then, List<TParams...>, Else, List<EParams...>>
        {
            using Result = Else<EParams...>;
        };
    }

}
