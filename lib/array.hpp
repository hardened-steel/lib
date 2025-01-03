#pragma once
#include <array>
#include <lib/raw.array.hpp>
#include <lib/concept.hpp>

namespace lib {
    template<class T, std::size_t ...Ia, std::size_t ...Ib>
    constexpr auto concat_impl(const std::array<T, sizeof...(Ia)>& a, std::index_sequence<Ia...>, const std::array<T, sizeof...(Ib)>& b, std::index_sequence<Ib...>) noexcept
    {
        return std::array<T, sizeof...(Ia) + sizeof...(Ib)>{a[Ia]..., b[Ib]...};
    }

    template<class T, std::size_t A, std::size_t B>
    constexpr auto concat(const std::array<T, A>& a, const std::array<T, B>& b) noexcept
    {
        return concat_impl(a, std::make_index_sequence<A>{}, b, std::make_index_sequence<B>{});
    }

    template<std::size_t ...Ia, std::size_t ...Ib>
    constexpr auto concat_impl(const std::array<char, sizeof...(Ia)>& a, std::index_sequence<Ia...>, RawArray<const char, sizeof...(Ib) + 1U> b, std::index_sequence<Ib...>) noexcept
    {
        return std::array<char, sizeof...(Ia) + sizeof...(Ib)>{a[Ia]..., b[Ib]...};
    }

    template<std::size_t A, std::size_t B>
    constexpr auto concat(const std::array<char, A>& a, RawArray<const char, B> b) noexcept
    {
        return concat_impl(a, std::make_index_sequence<A>{}, b, std::make_index_sequence<B - 1U>{});
    }

    template<std::size_t ...Ia, std::size_t ...Ib>
    constexpr auto concat_impl(RawArray<const char, sizeof...(Ia) + 1U> a, std::index_sequence<Ia...>, const std::array<char, sizeof...(Ib)>& b, std::index_sequence<Ib...>) noexcept
    {
        return std::array<char, sizeof...(Ia) + sizeof...(Ib)>{a[Ia]..., b[Ib]...};
    }

    template<std::size_t A, std::size_t B>
    constexpr auto concat(RawArray<const char, A> a, const std::array<char, B>& b) noexcept
    {
        return concat_impl(a, std::make_index_sequence<A - 1U>{}, b, std::make_index_sequence<B>{});
    }

    template<class Arg, class ...Ts, typename = Require<(sizeof...(Ts) > 1U)>>
    constexpr auto concat(const Arg& arg, const Ts& ...args) noexcept
    {
        return concat(arg, concat(args...));
    }

    template<class Arg>
    constexpr auto concat(const Arg& arg) noexcept
    {
        return arg;
    }

    template<std::size_t A, class Arg, class ...Ts>
    constexpr auto separated(RawArray<const char, A> separator, const Arg& arg, const Ts& ...args) noexcept
    {
        if constexpr(sizeof...(Ts)) {
            return concat(arg, separator, separated(separator, args...));
        } else {
            return arg;
        }
    }

    template<class T, std::size_t N, std::size_t ...I>
    constexpr auto array_cast(const T(&array)[N], std::index_sequence<I...>) noexcept // NOLINT
    {
        return std::array<T, N>{array[I]...};
    }

    template<class T, std::size_t N>
    constexpr auto array_cast(const T(&array)[N]) noexcept // NOLINT
    {
        return array_cast(array, std::make_index_sequence<N>{});
    }

    template<class T>
    class Span
    {
        T*          m_ptr  = nullptr;
        std::size_t m_size = 0;
    public:
        constexpr Span() = default;

        template<std::size_t N>
        constexpr Span(T(&array)[N]) noexcept // NOLINT
        : m_ptr(array), m_size(N)
        {}
        
        template<std::size_t N>
        constexpr Span(std::array<T, N>& array) noexcept
        : m_ptr(array.data()), m_size(array.size())
        {}

        constexpr Span(const Span& other) = default;
        constexpr Span& operator=(const Span& other) = default;

        template<class U, std::enable_if_t<std::is_convertible_v<T*, U*>, bool> = true>
        constexpr operator Span<U>() const noexcept
        {
            return Span<U>(m_ptr, m_size);
        }

        constexpr T& operator[](std::size_t index) const noexcept
        {
            return m_ptr[index];
        }

        constexpr auto data() const noexcept
        {
            return m_ptr;
        }
        constexpr auto size() const noexcept
        {
            return m_size;
        }
        constexpr auto begin() const noexcept
        {
            return m_ptr;
        }
        constexpr auto end() const noexcept
        {
            return m_ptr + m_size;
        }
    };
}
