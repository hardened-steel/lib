#pragma once
#include <tuple>
#include <lib/concept.hpp>

namespace lib {
    template<class T>
    struct IsTupleF
    {
        constexpr static inline bool value = false;
    };

    template<class T>
    constexpr inline bool is_tuple = IsTupleF<T>::value;

    template<class T>
    struct IsTupleF<const T&>
    {
        constexpr static inline bool value = is_tuple<T>;
    };

    template<class T>
    struct IsTupleF<T&&>
    {
        constexpr static inline bool value = is_tuple<T>;
    };

    template<class ...Ts>
    struct IsTupleF<std::tuple<Ts...>>
    {
        constexpr static inline bool value = true;
    };

    template<class Tuple, typename = Require<is_tuple<Tuple>>>
    constexpr auto&& head(Tuple&& tuple) noexcept
    {
        return std::get<0>(tuple);
    }

    namespace details {
        template<class Tuple, std::size_t ...I>
        constexpr auto tail(Tuple&& tuple, std::index_sequence<I...>) noexcept
        {
            return std::tie(std::get<I + 1>(tuple)...);
        }
    }

    template<class Tuple, typename = Require<is_tuple<Tuple>>>
    constexpr auto tail(Tuple&& tuple) noexcept
    {
        constexpr auto size = std::tuple_size_v<Tuple>;
        if constexpr (size > 1) {
            return details::tail(std::forward<Tuple>(tuple), std::make_index_sequence<size - 1>{});
        } else {
            return std::make_tuple();
        }
    }
}
