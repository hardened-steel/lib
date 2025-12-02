#pragma once
#include <lib/static.string.hpp>


namespace lib {
    namespace details::type_name {
        template <typename T>
        constexpr std::string_view get_type_name_impl() noexcept
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
            constexpr auto prefix = std::string_view{"get_type_name_impl<"};
            constexpr auto suffix = ">(void)";
            constexpr auto function = std::string_view{__FUNCSIG__};
        #else
        #   error Unsupported compiler
        #endif

        #if defined(__clang__) || defined(__GNUC__)
            const auto start = function.find(prefix) + prefix.size();
            const auto end = function.find(suffix);
            const auto size = end - start;

            return function.substr(start, size);
        #else
            const auto start = function.find(prefix) + prefix.size();
            const auto end = function.find(suffix);
            const auto size = end - start;

            const auto type = function.substr(start, size);
            return type.substr(type.find(' ') + 1);
        #endif
        }

        template <typename T>
        constexpr auto get_type_name() noexcept
        {
            constexpr auto pretty = get_type_name_impl<T>();
            constexpr auto size = pretty.size();
            return StaticString<size>(pretty.data());
        }
    }

    template <class T>
    struct TypeName
    {
        constexpr static inline auto name = details::type_name::get_type_name<T>();
    };

    template <class T>
    constexpr inline auto type_name = TypeName<T>::name;

    namespace details::type_name {
        template <auto V>
        struct ValueTag {};

        template <auto V>
        constexpr auto get_value_string_impl() noexcept
        {
            constexpr std::string_view pretty = lib::type_name<ValueTag<V>>;
            const std::string_view prefix = "ValueTag<";
            const std::string_view suffix = ">";

            const auto start = pretty.find(prefix) + prefix.size();
            const auto end = pretty.rfind(suffix);
            const auto size = end - start;

            return pretty.substr(start, size);
        }

        template <auto V>
        constexpr auto get_value_string() noexcept
        {
            constexpr auto pretty = get_value_string_impl<V>();
            constexpr auto size = pretty.size();
            return StaticString<size>(pretty.data());
        }
    }

    template <auto V>
    struct ValueString
    {
        constexpr static inline auto string = details::type_name::get_value_string<V>();
    };

    template <auto V>
    constexpr inline auto value_string = ValueString<V>::string;

    template <>
    struct TypeName<std::string_view>
    {
        constexpr static inline auto name = "std::string_view";
    };

    template <std::size_t N>
    struct TypeName<StaticString<N>>
    {
        constexpr static inline auto name = "lib::static.string[" + value_string<N> + "]";
    };

    template <class T, std::size_t N>
    struct TypeName<std::array<T, N>>
    {
        constexpr static inline auto name = "std::array<" + type_name<T> + ", " + value_string<N> + ">";
    };

    template <class T>
    struct TypeName<Span<T>>
    {
        constexpr static inline auto name = "lib::Span<" + type_name<T> + ">";
    };

    template <>
    struct TypeName<std::uint8_t>
    {
        constexpr static inline StaticString name = "u8";
    };

    template <>
    struct TypeName<std::int8_t>
    {
        constexpr static inline StaticString name = "i8";
    };

    template <>
    struct TypeName<std::uint16_t>
    {
        constexpr static inline StaticString name = "u16";
    };

    template <>
    struct TypeName<std::int16_t>
    {
        constexpr static inline StaticString name = "i16";
    };

    template <>
    struct TypeName<std::uint32_t>
    {
        constexpr static inline StaticString name = "u32";
    };

    template <>
    struct TypeName<std::int32_t>
    {
        constexpr static inline StaticString name = "i32";
    };

    template <>
    struct TypeName<std::uint64_t>
    {
        constexpr static inline StaticString name = "u64";
    };

    template <>
    struct TypeName<std::int64_t>
    {
        constexpr static inline StaticString name = "i64";
    };
}
