#pragma once
#include <cstddef>
#include <string_view>
#include <array>
#include <utility>

namespace lib {
    namespace details::type_name {
        template <typename T>
        constexpr std::string_view get_type_name() noexcept
        {
        #if defined(__clang__)
            constexpr auto prefix = std::string_view{"[T = "};
            constexpr auto suffix = "]";
            constexpr auto function = std::string_view{__PRETTY_FUNCTION__};
        #elif defined(__GNUC__)
            constexpr auto prefix = std::string_view{"with T = "};
            constexpr auto suffix = "; ";
            constexpr auto function = std::string_view{__PRETTY_FUNCTION__};
        #elif defined(_MSC_VER)
            constexpr auto prefix = std::string_view{"get_type_name<"};
            constexpr auto suffix = ">(void)";
            constexpr auto function = std::string_view{__FUNCSIG__};
        #else
        #   error Unsupported compiler
        #endif

            const auto start = function.find(prefix) + prefix.size();
            const auto end = function.find(suffix);
            const auto size = end - start;

            return function.substr(start, size);
        }

        template <std::size_t ...Is>
        constexpr std::array<char, sizeof...(Is)> create_array(std::string_view view, std::index_sequence<Is...>) noexcept
        {
            return {view[Is]...};
        }

        template <std::size_t N>
        constexpr std::array<char, N> create_array(std::string_view view) noexcept
        {
            return create_array(view, std::make_index_sequence<N>{});
        }

        template <typename T>
        constexpr auto get_type_name_array() noexcept
        {
            constexpr auto pretty = get_type_name<T>();
            constexpr auto size = pretty.size();
            return create_array<size>(pretty);
        }

        template<class T>
        constexpr inline auto type_name_array = get_type_name_array<T>();

        template<std::size_t Value>
        struct ValueTag {};

        template <std::size_t Value>
        constexpr auto get_value_string() noexcept
        {
            constexpr auto pretty = get_type_name<ValueTag<Value>>();
            constexpr auto prefix = std::string_view{"ValueTag<"};
            constexpr auto suffix = ">";
            
            const auto start = pretty.find(prefix) + prefix.size();
            const auto end = pretty.find(suffix);
            const auto size = end - start;

            return pretty.substr(start, size);
        }

        template <std::size_t Value>
        constexpr auto get_value_string_array() noexcept
        {
            constexpr auto pretty = get_value_string<Value>();
            constexpr auto size = pretty.size();
            return create_array<size>(pretty);
        }

        template <std::size_t Value>
        constexpr inline auto value_array = get_value_string_array<Value>();
    }

    template<class T>
    constexpr inline auto type_name_array = details::type_name::type_name_array<T>;

    template<class T>
    constexpr inline std::string_view type_name {
        details::type_name::type_name_array<T>.data(),
        details::type_name::type_name_array<T>.size()
    };

    template<std::size_t Value>
    constexpr inline auto tostring_array = details::type_name::value_array<Value>;

    template<std::size_t Value>
    constexpr inline std::string_view tostring {
        details::type_name::value_array<Value>.data(),
        details::type_name::value_array<Value>.size()
    };

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

    template<std::size_t ...Is>
    constexpr auto copy_from_view(std::string_view view, std::index_sequence<Is...>) noexcept
    {
        return std::array<char, sizeof...(Is)>{view[Is]...};
    }

    template<std::size_t N>
    constexpr auto copy_from_view(std::string_view view) noexcept
    {
        return copy_from_view(view, std::make_index_sequence<N>{});
    }
}
