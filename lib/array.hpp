#pragma once
#include <array>
#include <lib/concept.hpp>

namespace lib {
    namespace details {
        template<class T, std::size_t N>
        using RawArray = T(&)[N];
    }

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
    constexpr auto concat_impl(const std::array<char, sizeof...(Ia)>& a, std::index_sequence<Ia...>, details::RawArray<const char, sizeof...(Ib) + 1u> b, std::index_sequence<Ib...>) noexcept
    {
        return std::array<char, sizeof...(Ia) + sizeof...(Ib)>{a[Ia]..., b[Ib]...};
    }

    template<std::size_t A, std::size_t B>
    constexpr auto concat(const std::array<char, A>& a, details::RawArray<const char, B> b) noexcept
    {
        return concat_impl(a, std::make_index_sequence<A>{}, b, std::make_index_sequence<B - 1u>{});
    }

    template<std::size_t ...Ia, std::size_t ...Ib>
    constexpr auto concat_impl(details::RawArray<const char, sizeof...(Ia) + 1u> a, std::index_sequence<Ia...>, const std::array<char, sizeof...(Ib)>& b, std::index_sequence<Ib...>) noexcept
    {
        return std::array<char, sizeof...(Ia) + sizeof...(Ib)>{a[Ia]..., b[Ib]...};
    }

    template<std::size_t A, std::size_t B>
    constexpr auto concat(details::RawArray<const char, A> a, const std::array<char, B>& b) noexcept
    {
        return concat_impl(a, std::make_index_sequence<A - 1u>{}, b, std::make_index_sequence<B>{});
    }

    template<class Arg, class ...Ts, typename = Require<(sizeof...(Ts) > 1u)>>
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
    constexpr auto separated(details::RawArray<const char, A> separator, const Arg& arg, const Ts& ...args) noexcept
    {
        if constexpr(sizeof...(Ts)) {
            return concat(arg, separator, separated(separator, args...));
        } else {
            return arg;
        }
    }

    template<class T, std::size_t N, std::size_t ...I>
    constexpr auto array_cast(const T(&array)[N], std::index_sequence<I...>) noexcept
    {
        return std::array<T, N>{array[I]...};
    }

    template<class T, std::size_t N>
    constexpr auto array_cast(const T(&array)[N]) noexcept
    {
        return array_cast(array, std::make_index_sequence<N>{});
    }

    template<class T>
    class span
    {
        T*          m_ptr  = nullptr;
        std::size_t m_size = 0;
    public:
        constexpr span() = default;

        template<std::size_t N>
        constexpr span(T(&array)[N]) noexcept
        : m_ptr(array), m_size(N)
        {}
        
        template<std::size_t N>
        constexpr span(std::array<T, N>& array) noexcept
        : m_ptr(array.data()), m_size(array.size())
        {}

        constexpr span(const span& other) = default;
        constexpr span& operator=(const span& other) = default;

        constexpr operator span<const T>() const noexcept
        {
            return span<const T>(m_ptr, m_size);
        }

        constexpr const T& operator[](std::size_t index) const noexcept
        {
            return m_ptr[index];
        }
        constexpr T& operator[](std::size_t index) noexcept
        {
            return m_ptr[index];
        }

        constexpr auto data() noexcept
        {
            return m_ptr;
        }
        constexpr auto size() noexcept
        {
            return m_size;
        }
        constexpr auto begin() noexcept
        {
            return m_ptr;
        }
        constexpr auto end() noexcept
        {
            return m_ptr + m_size;
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

    template<class T>
    class span<const T>
    {
        const T*    m_ptr;
        std::size_t m_size;
    public:
        template<std::size_t N>
        constexpr span(const T(&array)[N]) noexcept
        : m_ptr(array), m_size(N)
        {}

        template<class C>
        constexpr span(const C& container) noexcept
        : m_ptr(container.data()), m_size(container.size())
        {}

        constexpr span(std::initializer_list<T> list) noexcept
        : m_ptr(list.begin()), m_size(list.size())
        {}

        constexpr span(const span& other) = default;
        constexpr span& operator=(const span& other) = default;

        constexpr const T& operator[](std::size_t index) const noexcept
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
