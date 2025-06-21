#pragma once
#include <cstddef>

namespace lib::typetraits {

    template <class ...Ts>
    struct List
    {
        constexpr static inline std::size_t size = sizeof...(Ts);
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
}
