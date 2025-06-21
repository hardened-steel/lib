#pragma once
#include <string_view>
#include <lib/array.hpp>

namespace lib {

    template <std::size_t N>
    class StaticString;

    template <std::size_t Lhs, std::size_t Rhs>
    constexpr auto operator+ (const StaticString<Lhs>& lhs, const StaticString<Rhs>& rhs) noexcept;

    template <std::size_t N>
    class StaticString
    {
        template <std::size_t Lhs, std::size_t Rhs>
        friend constexpr auto operator+ (const StaticString<Lhs>& lhs, const StaticString<Rhs>& rhs) noexcept;

    public:
        std::array<char, N> string;

    private:
        template <std::size_t ...I>
        constexpr StaticString(const char* str, std::index_sequence<I...>) noexcept
        : string{str[I]...}
        {}

    public:
        constexpr StaticString(const char* str) noexcept
        : StaticString(str, std::make_index_sequence<N>{})
        {}

        constexpr StaticString(const std::array<char, N>& string) noexcept
        : string(string)
        {}

        constexpr StaticString(const StaticString& other) noexcept
        : string(other.string)
        {}

        constexpr StaticString& operator=(const StaticString& other) = default;

        constexpr auto data() noexcept
        {
            return string.data();
        }

        constexpr auto data() const noexcept
        {
            return string.data();
        }

        constexpr auto size() const noexcept
        {
            return string.size();
        }

        constexpr operator std::string_view() const noexcept
        {
            return std::string_view(string.data(), string.size());
        }

        constexpr const char& operator[](std::size_t index) const noexcept
        {
            return string[index];
        }

        constexpr char& operator[](std::size_t index) noexcept
        {
            return string[index];
        }

    public:
        friend std::ostream& operator<<(std::ostream& stream, const StaticString& string)
        {
            return stream << std::string_view(string);
        }
    };

    template <std::size_t N>
    StaticString(RawArray<const char, N>) -> StaticString<N - 1>;

    template <std::size_t N>
    StaticString(const std::array<char, N>&) -> StaticString<N>;

    template <std::size_t N>
    constexpr auto string(RawArray<const char, N> str) noexcept
    {
        return StaticString<N - 1>(str);
    }

    template <std::size_t Lhs, std::size_t Rhs>
    constexpr auto operator+ (const StaticString<Lhs>& lhs, const StaticString<Rhs>& rhs) noexcept
    {
        return StaticString(lib::concat(lhs.string, rhs.string));
    }

    template <std::size_t Lhs, std::size_t Rhs>
    constexpr auto operator+ (const StaticString<Lhs>& lhs, RawArray<const char, Rhs> rhs) noexcept
    {
        return StaticString(lib::concat(lhs.string, rhs));
    }

    template <std::size_t Lhs, std::size_t Rhs>
    constexpr auto operator+ (RawArray<const char, Lhs>& lhs, const StaticString<Rhs>& rhs) noexcept
    {
        return StaticString(lib::concat(lhs, rhs.string));
    }

    template <std::size_t Lhs, std::size_t Rhs>
    constexpr auto operator == (const StaticString<Lhs>& lhs, const StaticString<Rhs>& rhs) noexcept
    {
        return static_cast<std::string_view>(lhs) == static_cast<std::string_view>(rhs);
    }

    template <std::size_t Lhs, std::size_t Rhs>
    constexpr auto operator == (const StaticString<Lhs>& lhs, RawArray<const char, Rhs> rhs) noexcept
    {
        return static_cast<std::string_view>(lhs) == static_cast<std::string_view>(rhs);
    }

    template <std::size_t Lhs, std::size_t Rhs>
    constexpr auto operator == (RawArray<const char, Lhs>& lhs, const StaticString<Rhs>& rhs) noexcept
    {
        return static_cast<std::string_view>(lhs) == static_cast<std::string_view>(rhs);
    }

    template <std::size_t Lhs, std::size_t Rhs>
    constexpr auto operator != (const StaticString<Lhs>& lhs, const StaticString<Rhs>& rhs) noexcept
    {
        return static_cast<std::string_view>(lhs) != static_cast<std::string_view>(rhs);
    }

    template <std::size_t Lhs, std::size_t Rhs>
    constexpr auto operator != (const StaticString<Lhs>& lhs, RawArray<const char, Rhs> rhs) noexcept
    {
        return static_cast<std::string_view>(lhs) != static_cast<std::string_view>(rhs);
    }

    template <std::size_t Lhs, std::size_t Rhs>
    constexpr auto operator != (RawArray<const char, Lhs>& lhs, const StaticString<Rhs>& rhs) noexcept
    {
        return static_cast<std::string_view>(lhs) != static_cast<std::string_view>(rhs);
    }

    template <std::size_t Lhs, std::size_t Rhs>
    constexpr auto operator < (const StaticString<Lhs>& lhs, const StaticString<Rhs>& rhs) noexcept
    {
        return static_cast<std::string_view>(lhs) < static_cast<std::string_view>(rhs);
    }
}
