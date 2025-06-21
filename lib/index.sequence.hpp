#pragma once
#include <array>
#include <utility>

namespace lib::indexes {
    template <std::size_t NF, std::size_t NL>
    constexpr std::size_t length = (NF > NL) ? NF - NL + 1 : NL - NF + 1;

    static_assert(length<0, 1> == 2);
    static_assert(length<3, 5> == 3);

    template <std::size_t V, std::size_t ...Is>
    constexpr auto add(std::index_sequence<Is...>) noexcept
    {
        return std::index_sequence<(Is + V)...>{};
    }

    static_assert(std::is_same_v<decltype(add<1>(std::index_sequence<0, 1, 2>())), std::index_sequence<1, 2, 3>>);

    template <std::size_t V, std::size_t ...Is>
    constexpr auto neg(std::index_sequence<Is...>) noexcept
    {
        return std::index_sequence<(V - Is)...>{};
    }

    static_assert(std::is_same_v<decltype(neg<3>(std::index_sequence<3, 2, 1>())), std::index_sequence<0, 1, 2>>);

    template <std::size_t NF, std::size_t NL>
    constexpr auto range() noexcept
    {
        constexpr std::size_t nsize = length<NF, NL>;
        if constexpr (NF > NL) {
            return add<NL>(std::make_index_sequence<nsize>{});
        } else {
            return neg<NL>(std::make_index_sequence<nsize>{});
        }
    }

    static_assert(std::is_same_v<decltype(range<1, 3>()), std::index_sequence<3, 2, 1>>);
    static_assert(std::is_same_v<decltype(range<3, 1>()), std::index_sequence<1, 2, 3>>);

    template <std::size_t ...Is, std::size_t ...Indices>
    constexpr auto islice(std::index_sequence<Is...>, std::index_sequence<Indices...>) noexcept
    {
        using Array = std::array<std::size_t, sizeof...(Indices)>;
        return std::index_sequence<std::get<Is>(Array{{Indices...}})...>();
    }
}
