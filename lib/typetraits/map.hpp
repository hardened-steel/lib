#pragma once
#include <cstddef>
#include <lib/typetraits/set.hpp>

namespace lib::typetraits {

    template<class ...Ts>
    struct Map
    {
        constexpr static inline std::size_t size = sizeof...(Ts);
    };

    template<class EKey, class EValue>
    struct Map<EKey, EValue>
    {
        using Key = EKey;
        using Value = EValue;
    };

    template<class AKey, class AValue, class BKey, class BValue>
    struct Map<Map<AKey, AValue>, Map<BKey, BValue>>
    {
        constexpr static inline std::size_t size = 2;
    };

    namespace impl {

        template<class ...Ts>
        struct ToListF<Map<Ts...>>
        {
            using Result = List<Ts...>;
        };

        template<class List>
        struct MapFromListF;

        template<class ...Ts>
        struct MapFromListF<List<Ts...>>
        {
            using Result = Map<Ts...>;
        };

        template<class List>
        using MapFromList = typename MapFromListF<List>::Result;

        template<class EKey, class EValue, class ...Tail, class IKey, class IValue>
        struct InsertF<Map<Map<EKey, EValue>, Tail...>, Map<IKey, IValue>>
        {
            using Head = Map<EKey, EValue>;
            using Result = std::conditional_t<
                lib::type_name<EKey> < lib::type_name<IKey>,
                Map<Map<IKey, IValue>, Head, Tail...>,
                MapFromList<
                    InsertFront<
                        ToList<
                            Insert<Map<Tail...>, Map<IKey, IValue>>
                        >,
                        Head
                    >
                >
            >;
        };

        template<class Key, class EValue, class ...Tail, class IValue>
        struct InsertF<Map<Map<Key, EValue>, Tail...>, Map<Key, IValue>>
        {
            using Result = Map<Map<Key, IValue>, Tail...>;
        };

        template<class T>
        struct InsertF<Map<>, T>
        {
            using Result = Map<T>;
        };
    }

    namespace impl {
        template<class ...Ts, std::size_t Index>
        struct GetF<Map<Ts...>, Index>
        {
            using Result = typename Get<List<Ts...>, Index>::Value;
        };
    }

    namespace impl {
        template<class EKey, class EValue, class ...Tail, class IKey>
        struct IndexF<Map<Map<EKey, EValue>, Tail...>, IKey>
        {
            static inline constexpr std::size_t value = index<Map<Tail...>, IKey> + 1;
        };

        template<class EKey, class EValue, class ...Tail>
        struct IndexF<Map<Map<EKey, EValue>, Tail...>, EKey>
        {
            static inline constexpr std::size_t value = 0;
        };

        template<class Key>
        struct IndexF<Map<>, Key>
        {
            static inline constexpr std::size_t value = 0;
        };
    }

    namespace impl {
        template<class Map, class Key>
        struct FindF
        {
            using Result = Get<Map, index<Map, Key>>;
        };

        template<class Map, class Key>
        using Find = typename FindF<Map, Key>::Result;
    }

    template<class Map, class Key>
    using Find = impl::Find<Map, Key>;

    namespace impl {
        template<class Head, class ...Tail, std::size_t Index>
        struct RemoveF<Map<Head, Tail...>, Index>
        {
            static_assert(Index < List<Head, Tail...>::size);
            using Result = Insert<Remove<Map<Tail...>, Index - 1>, Head>;
        };

        template<class Head, class ...Tail>
        struct RemoveF<Map<Head, Tail...>, 0>
        {
            using Result = Map<Tail...>;
        };
    }

    namespace impl {

        template<class EKey, class EValue, class ...Tail, class IKey>
        struct EraseF<Map<Map<EKey, EValue>, Tail...>, IKey>
        {
            using Head = Map<EKey, EValue>;
            using Result = MapFromList<
                InsertFront<
                    ToList<
                        Erase<Map<Tail...>, IKey>
                    >,
                    Head
                >
            >;
        };

        template<class IKey, class ...Tail>
        struct EraseF<Map<IKey, Tail...>, IKey>
        {
            using Result = Map<Tail...>;
        };

        template<class IKey>
        struct EraseF<Map<>, IKey>
        {
            using Result = Map<>;
        };
    }
}
