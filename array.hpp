#pragma once
#include <array>

namespace lib {
    template<std::size_t ...Ia, std::size_t ...Ib>
    constexpr auto concat(const std::array<char, sizeof...(Ia)>& a, std::index_sequence<Ia...>, const std::array<char, sizeof...(Ib)>& b, std::index_sequence<Ib...>) noexcept
    {
        return std::array<char, sizeof...(Ia) + sizeof...(Ib)>{a[Ia]..., b[Ib]...};
    }

    template<std::size_t A, std::size_t B>
    constexpr auto concat(const std::array<char, A>& a, const std::array<char, B>& b) noexcept
    {
        return concat(a, std::make_index_sequence<A>{}, b, std::make_index_sequence<B>{});
    }

    template<std::size_t ...Ia, std::size_t ...Ib>
    constexpr auto concat(const std::array<char, sizeof...(Ia)>& a, std::index_sequence<Ia...>, const char (&b)[sizeof...(Ib) + 1u], std::index_sequence<Ib...>) noexcept
    {
        return std::array<char, sizeof...(Ia) + sizeof...(Ib)>{a[Ia]..., b[Ib]...};
    }

    template<std::size_t A, std::size_t B>
    constexpr auto concat(const std::array<char, A>& a, const char (&b)[B]) noexcept
    {
        return concat(a, std::make_index_sequence<A>{}, b, std::make_index_sequence<B - 1u>{});
    }

    template<std::size_t ...Ia, std::size_t ...Ib>
    constexpr auto concat(const char (&a)[sizeof...(Ia) + 1u], std::index_sequence<Ia...>, const std::array<char, sizeof...(Ib)>& b, std::index_sequence<Ib...>) noexcept
    {
        return std::array<char, sizeof...(Ia) + sizeof...(Ib)>{a[Ia]..., b[Ib]...};
    }

    template<std::size_t A, std::size_t B>
    constexpr auto concat(const char (&a)[A], const std::array<char, B>& b) noexcept
    {
        return concat(a, std::make_index_sequence<A - 1u>{}, b, std::make_index_sequence<B>{});
    }

    template<class Arg, class ...Ts>
    constexpr auto concat(const Arg& arg, const Ts& ...args) noexcept
    {
        return concat(arg, concat(args...));
    }

    template<std::size_t A, class Arg, class ...Ts>
    constexpr auto separated(const char (&separator)[A], const Arg& arg, const Ts& ...args) noexcept
    {
        if constexpr(sizeof...(Ts)) {
            return concat(arg, separator, separated(separator, args...));
        } else {
            return arg;
        }
    }
}