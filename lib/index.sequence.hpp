#pragma once
#include <array>
#include <utility>

namespace lib::indexes {
    template<std::size_t NF, std::size_t NL>
    constexpr std::size_t length = (NF > NL) ? NF - NL + 1 : NL - NF + 1;

    template <std::size_t V, std::size_t ...Is>
    constexpr auto add(std::index_sequence<Is...>) noexcept
    {
        return std::index_sequence<(Is + V)...>{};
    }

    template <std::size_t V, std::size_t ...Is>
    constexpr auto neg(std::index_sequence<Is...>) noexcept
    {
        return std::index_sequence<(V - Is)...>{};
    }

    template<std::size_t NF, std::size_t NL>
    constexpr auto range() noexcept
    {
        constexpr std::size_t nsize = length<NF, NL>;
        if constexpr (NF > NL) {
            return add<NL>(std::make_index_sequence<nsize>{});
        } else {
            return neg<NL>(std::make_index_sequence<nsize>{});
        }
    }

    template<std::size_t ...Is, std::size_t ...Indices>
    constexpr auto islice(std::index_sequence<Is...>, std::index_sequence<Indices...>) noexcept
    {
        using Array = std::array<std::size_t, sizeof...(Indices)>;
        return std::index_sequence<std::get<Is>(Array{{Indices...}})...>();
    }
}
