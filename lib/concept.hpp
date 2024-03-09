#pragma once
#include <type_traits>

namespace lib {
    template<bool ...Conditions>
    using Require = std::enable_if_t<(Conditions && ...), void>;

    // Primary template handles all types not supporting the operation.
    template <typename, template <typename> class, typename = std::void_t<>>
    struct Detect: std::false_type {};

    // Specialization recognizes/validates only types supporting the archetype.
    template <typename T, template <typename> class Op>
    struct Detect<T, Op, std::void_t<Op<T>>>: std::true_type {};

    template <typename T, template <typename> class Op>
    constexpr inline bool detect = Detect<T, Op>{};
    /* using example:

        template <typename T>
        using has_toString_t = decltype(std::declval<T>().toString());

        template<class T>
        constexpr inline bool has_toString = lib::detect<T, has_toString_t>;

    */

}

#define LIB_TYPE_HAS_MEMBER(Type, Member) lib::detect<Type, decltype(std::declval<Type>().Member)>