#pragma once
#include <cstddef>
#include <lib/typetraits/list.hpp>
#include <lib/typetraits/for.hpp>
#include <lib/typename.hpp>
#include <type_traits>


namespace lib::typetraits {

    template <class ...Ts>
    struct Set
    {
        constexpr static inline std::size_t size = sizeof...(Ts);
        using ToList = List<Ts...>;
    };

    namespace impl {
        template <class Set>
        struct ToListF;

        template <class ...Ts>
        struct ToListF<Set<Ts...>>
        {
            using Result = List<Ts...>;
        };

        template <class Set>
        using ToList = typename ToListF<Set>::Result;

        template <class List>
        struct SetFromListF;

        template <class ...Ts>
        struct SetFromListF<List<Ts...>>
        {
            using Result = Set<Ts...>;
        };

        template <class List>
        using SetFromList = typename SetFromListF<List>::Result;

        template <class Set, class T>
        struct InsertF;

        template <class Set, class T>
        using Insert = typename InsertF<Set, T>::Result;

        template <class Head, class ...Tail, class T>
        struct InsertF<Set<Head, Tail...>, T>
        {
            using Result = std::conditional_t<
                lib::type_name<T> < lib::type_name<Head>,
                Set<T, Head, Tail...>,
                SetFromList<
                    InsertFront<
                        ToList<
                            Insert<Set<Tail...>, T>
                        >,
                        Head
                    >
                >
            >;
        };

        template <class ...Tail, class T>
        struct InsertF<Set<T, Tail...>, T>
        {
            using Result = Set<T, Tail...>;
        };

        template <class T>
        struct InsertF<Set<>, T>
        {
            using Result = Set<T>;
        };
    }

    template <class Set, class T>
    using Insert = impl::Insert<Set, T>;

    namespace impl {
        template <class Lhs, class Rhs>
        struct MergeBinaryF;

        template <class Lhs, class Rhs>
        using MergeBinary = typename MergeBinaryF<Lhs, Rhs>::Result;

        template <class Lhs, class Head, class ...Tail>
        struct MergeBinaryF<Lhs, Set<Head, Tail...>>
        {
            using Result = MergeBinary<Insert<Lhs, Head>, Set<Tail...>>;
        };

        template <class ...Types>
        struct MergeBinaryF<Set<Types...>, Set<>>
        {
            using Result = Set<Types...>;
        };

        template <class ...Sets>
        struct MergeF;

        template <class ...Sets>
        using Merge = typename MergeF<Sets...>::Result;

        template <class Lhs, class Head, class ...Tail>
        struct MergeF<Lhs, Head, Tail...>
        {
            using Result = typename MergeF<MergeBinary<Lhs, Head>, Tail...>::Result;
        };

        template <class Lhs, class Rhs>
        struct MergeF<Lhs, Rhs>
        {
            using Result = MergeBinary<Lhs, Rhs>;
        };
    }

    template <class ...Sets>
    using Merge = impl::Merge<Sets...>;

    namespace impl {
        template <class ...TList>
        struct CreateSetF
        {
            using Result = typename CreateSetF<List<TList...>>::Result;
        };

        template <class T, class ...Ts>
        struct CreateSetF<List<T, Ts...>>
        {
            using Set = typename CreateSetF<List<Ts...>>::Result;
            using Result = Insert<Set, T>;
        };

        template <class T>
        struct CreateSetF<List<T>>
        {
            using Result = Set<T>;
        };
    }

    template <class ...TList>
    using CreateSet = typename impl::CreateSetF<TList...>::Result;

    namespace impl {
        template <class ...Ts, std::size_t Index>
        struct GetF<Set<Ts...>, Index>
        {
            using Result = Get<List<Ts...>, Index>;
        };
    }

    namespace impl {
        template <class Set, class T>
        struct IndexF;

        template <class Set, class T>
        constexpr auto index = IndexF<Set, T>::value;

        template <class Head, class ...Tail, class T>
        struct IndexF<Set<Head, Tail...>, T>
        {
            static inline constexpr std::size_t value = index<Set<Tail...>, T> + 1;
        };

        template <class T, class ...Tail>
        struct IndexF<Set<T, Tail...>, T>
        {
            static inline constexpr std::size_t value = 0;
        };

        template <class T>
        struct IndexF<Set<>, T>
        {
            static inline constexpr std::size_t value = 0;
        };
    }

    template <class Set, class T>
    constexpr auto index = impl::index<Set, T>;

    namespace impl {
        template <class Head, class ...Tail, std::size_t Index>
        struct RemoveF<Set<Head, Tail...>, Index>
        {
            static_assert(Index < List<Head, Tail...>::size);
            using Result = Insert<Remove<Set<Tail...>, Index - 1>, Head>;
        };

        template <class Head, class ...Tail>
        struct RemoveF<Set<Head, Tail...>, 0>
        {
            using Result = Set<Tail...>;
        };
    }

    namespace impl {
        template <class ISet, class T>
        struct ExistsF
        {
            static inline constexpr bool value = index<ISet, T> < ISet::size;
        };

        template <class Set, class T>
        constexpr bool exists = ExistsF<Set, T>::value;
    }

    template <class Set, class T>
    constexpr bool exists = impl::exists<Set, T>;

    namespace impl {
        template <class ISet, class T>
        struct EraseF;

        template <class Set, class T>
        using Erase = typename EraseF<Set, T>::Result;

        template <class Head, class ...Tail, class T>
        struct EraseF<Set<Head, Tail...>, T>
        {
            using Result = SetFromList<
                InsertFront<
                    ToList<
                        Erase<Set<Tail...>, T>
                    >,
                    Head
                >
            >;
        };

        template <class T, class ...Tail>
        struct EraseF<Set<T, Tail...>, T>
        {
            using Result = Set<Tail...>;
        };

        template <class T>
        struct EraseF<Set<>, T>
        {
            using Result = Set<>;
        };
    }

    template <class Set, class T>
    using Erase = impl::Erase<Set, T>;

    namespace impl {
        template <class Set, template <typename> class F>
        struct ApplyF;

        template <template <typename> class F, class ...Ts>
        struct ApplyF<Set<Ts...>, F>
        {
            using Result = CreateSet<F<Ts>...>;
        };
    }

    template <class Set, template <typename> class F>
    using Apply = typename impl::ApplyF<Set, F>::Result;

    namespace test {
        using EmptySet = Set<>;
        static_assert(std::is_same_v<Set<int>, Insert<EmptySet, int>>);
        static_assert(std::is_same_v<Set<int>, Insert<Set<int>, int>>);
        static_assert(std::is_same_v<Set<float, int>, Insert<Set<int>, float>>);
        static_assert(std::is_same_v<Set<float, int, void>, Insert<Set<float, int>, void>>);

        using S = Insert<Insert<Insert<EmptySet, void>, int>, float>;
        static_assert(std::is_same_v<Get<S, 0>, float>);
        static_assert(std::is_same_v<Get<S, 1>, int>);
        static_assert(std::is_same_v<Get<S, 2>, void>);

        static_assert(std::is_same_v<Insert<Insert<Insert<S, int>, float>, void>, Insert<Insert<Insert<S, float>, int>, void>>);
        static_assert(lib::typetraits::index<Set<float, int>, int> == 1);
        static_assert(lib::typetraits::index<Set<float, int>, float> == 0);
        static_assert(lib::typetraits::index<S, unsigned> == S::size);

        static_assert(exists<S, float>);
        static_assert(!exists<S, unsigned>);
        static_assert(std::is_same_v<Erase<S, float>, Set<int, void>>);
        static_assert(std::is_same_v<Erase<S, unsigned>, S>);

        static_assert(std::is_same_v<S, Merge<EmptySet, S>>);
        static_assert(std::is_same_v<S, Merge<EmptySet, EmptySet, Insert<Insert<S, float>, int>, Set<void>>>);
        static_assert(std::is_same_v<S, Merge<EmptySet, Insert<Set<float>, int>, EmptySet, Set<void>>>);
    }
}
