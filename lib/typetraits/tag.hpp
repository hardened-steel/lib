#pragma once


namespace lib::typetraits {
    template <class ...Ts>
    struct TTag {};

    template <class ...Ts>
    constexpr inline TTag<Ts...> tag_t;

    template <auto ...Vs>
    struct VTag {};

    template <auto ...Vs>
    constexpr inline VTag<Vs...> tag_v;
}
