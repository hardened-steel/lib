#pragma once
#include <tuple>
#include <cstddef>


namespace lib::typetraits {

    template <class ...Ts>
    struct List
    {
        constexpr static inline std::size_t size = sizeof...(Ts);
        using Tuple = std::tuple<Ts...>;
    };

    namespace impl {
        template <class List>
        struct HeadF;

        template <class List>
        using Head = typename HeadF<List>::Result;

        template <class Head, class ...Ts>
        struct HeadF<List<Head, Ts...>>
        {
            using Result = Head;
        };
    }

    template <class List>
    using Head = impl::Head<List>;

    namespace impl {
        template <class List>
        struct TailF;

        template <class List>
        using Tail = typename TailF<List>::Result;

        template <class Head, class ...Ts>
        struct TailF<List<Head, Ts...>>
        {
            using Result = List<Ts...>;
        };
    }

    template <class List>
    using Tail = impl::Tail<List>;

    namespace impl {
        template <class List, class T>
        struct InsertBackF;

        template <class List, class T>
        using InsertBack = typename InsertBackF<List, T>::Result;

        template <class ...Ts, class T>
        struct InsertBackF<List<Ts...>, T>
        {
            using Result = List<Ts..., T>;
        };
    }

    template <class List, class T>
    using InsertBack = impl::InsertBack<List, T>;

    namespace impl {
        template <class List, class T>
        struct InsertFrontF;

        template <class List, class T>
        using InsertFront = typename InsertFrontF<List, T>::Result;

        template <class ...Ts, class T>
        struct InsertFrontF<List<Ts...>, T>
        {
            using Result = List<T, Ts...>;
        };
    }

    template <class List, class T>
    using InsertFront = impl::InsertFront<List, T>;

    namespace impl {
        template <class List, std::size_t Index>
        struct GetF;

        template <class List, std::size_t Index>
        using Get = typename GetF<List, Index>::Result;

        template <class Head, class ...Tail, std::size_t Index>
        struct GetF<List<Head, Tail...>, Index>
        {
            static_assert(Index < List<Head, Tail...>::size);
            using Result = Get<List<Tail...>, Index - 1>;
        };

        template <class Head, class ...Tail>
        struct GetF<List<Head, Tail...>, 0>
        {
            using Result = Head;
        };
    }

    template <class List, std::size_t Index>
    using Get = impl::Get<List, Index>;

    template <class List>
    using Last = impl::Get<List, List::size - 1>;

    namespace impl {
        template <class ...Lists>
        struct ConcatF;

        template <class ...Lists>
        using Concat = typename ConcatF<Lists...>::Result;

        template <class ...LhsTs, class ...RhsTs>
        struct ConcatF<List<LhsTs...>, List<RhsTs...>>
        {
            using Result = List<LhsTs..., RhsTs...>;
        };

        template <class ...Ts>
        struct ConcatF<List<Ts...>>
        {
            using Result = List<Ts...>;
        };

        template <>
        struct ConcatF<>
        {
            using Result = List<>;
        };

        template <class ...HeadTs, class ...Tail>
        struct ConcatF<List<HeadTs...>, Tail...>
        {
            using Result = typename ConcatF<List<HeadTs...>, typename ConcatF<Tail...>::Result>::Result;
        };
    }

    template <class ...Lists>
    using Concat = typename impl::ConcatF<Lists...>::Result;

        namespace impl {
        template <class List, std::size_t Index, class Value>
        struct AssignF;

        template <class List, std::size_t Index, class Value>
        using Assign = typename AssignF<List, Index, Value>::Result;

        template <class Head, class ...Tail, std::size_t Index, class Value>
        struct AssignF<List<Head, Tail...>, Index, Value>
        {
            static_assert(Index < List<Head, Tail...>::size);
            using Result = Concat<List<Head>, Assign<List<Tail...>, Index - 1, Value>>;
        };

        template <class Head, class ...Tail, class Value>
        struct AssignF<List<Head, Tail...>, 0, Value>
        {
            using Result = List<Value>;
        };
    }

    template <class List, std::size_t Index, class Value>
    using Assign = impl::Assign<List, Index, Value>;

    namespace impl {
        template <class List, std::size_t Index>
        struct RemoveF;

        template <class List, std::size_t Index>
        using Remove = typename RemoveF<List, Index>::Result;

        template <class Head, class ...Tail, std::size_t Index>
        struct RemoveF<List<Head, Tail...>, Index>
        {
            static_assert(Index < List<Head, Tail...>::size);
            using Result = InsertFront<Remove<List<Tail...>, Index - 1>, Head>;
        };

        template <class Head, class ...Tail>
        struct RemoveF<List<Head, Tail...>, 0>
        {
            using Result = List<Tail...>;
        };
    }

    template <class List, std::size_t Index>
    using Remove = impl::Remove<List, Index>;

    namespace impl {
        template <class List, std::size_t Count>
        struct PopBackF;

        template <class List, std::size_t Count>
        using PopBack = typename PopBackF<List, Count>::Result;

        template <class List, std::size_t Count>
        struct PopBackF
        {
            using Result = PopBack<Remove<List, List::size - 1>, Count - 1>;
        };

        template <class List>
        struct PopBackF<List, 0>
        {
            using Result = List;
        };
    }

    template <class List, std::size_t Count = 1>
    using PopBack = impl::PopBack<List, Count>;

    namespace impl {
        template <class List, std::size_t Count>
        struct PopFrontF;

        template <class List, std::size_t Count>
        using PopFront = typename PopFrontF<List, Count>::Result;

        template <class List, std::size_t Count>
        struct PopFrontF
        {
            using Result = PopFront<Remove<List, 0>, Count - 1>;
        };

        template <class List>
        struct PopFrontF<List, 0>
        {
            using Result = List;
        };
    }

    template <class List, std::size_t Count = 1>
    using PopFront = impl::PopFront<List, Count>;

    namespace test {
        using EmptyList = List<>;
        static_assert(EmptyList::size == 0);
        static_assert(std::is_same_v<InsertBack<EmptyList, void>, List<void>>);
        static_assert(std::is_same_v<InsertFront<EmptyList, void>, List<void>>);
        static_assert(std::is_same_v<InsertFront<EmptyList, void>, InsertBack<EmptyList, void>>);

        using L1 = List<int>;
        static_assert(L1::size == 1);
        using L2 = InsertBack<L1, void>;
        static_assert(L2::size == 2);
        static_assert(std::is_same_v<InsertBack<L1, void>, List<int, void>>);
        static_assert(std::is_same_v<InsertFront<L1, void>, List<void, int>>);

        static_assert(std::is_same_v<Get<L2, 0>, int>);
        static_assert(std::is_same_v<Get<L2, 1>, void>);

        static_assert(std::is_same_v<L1, Concat<L1, EmptyList>>);
        static_assert(std::is_same_v<L2, Concat<EmptyList, L2>>);
        static_assert(std::is_same_v<List<int, void, float>, Concat<L2, List<float>>>);
        static_assert(std::is_same_v<List<int, int, int>, Concat<L1, L1, L1>>);
        static_assert(std::is_same_v<L1, Concat<L1>>);

        static_assert(std::is_same_v<L1, Remove<L2, 1>>);
        static_assert(std::is_same_v<EmptyList, Remove<L1, 0>>);
    }
}
