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

        template<auto Value>
        struct ValueTag {};

        template <auto Value>
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

        template <auto Value>
        constexpr auto get_value_string_array() noexcept
        {
            constexpr auto pretty = get_value_string<Value>();
            constexpr auto size = pretty.size();
            return create_array<size>(pretty);
        }

        template <auto Value>
        constexpr inline auto value_array = get_value_string_array<Value>();
    }

    template<class T>
    constexpr inline auto type_name_array = details::type_name::type_name_array<T>;

    template<class T>
    constexpr inline std::string_view type_name {
        details::type_name::type_name_array<T>.data(),
        details::type_name::type_name_array<T>.size()
    };

    template<auto Value>
    constexpr inline auto tostring_array = details::type_name::value_array<Value>;

    template<auto Value>
    constexpr inline std::string_view tostring {
        details::type_name::value_array<Value>.data(),
        details::type_name::value_array<Value>.size()
    };

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
