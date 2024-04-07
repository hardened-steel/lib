#pragma once
#include <lib/array.hpp>
#include <string_view>
#include <utility>
#include <ostream>

namespace lib {

    template <std::size_t N>
    class StaticString;

    template<std::size_t A, std::size_t B>
    constexpr auto operator+ (const StaticString<A>& a, const StaticString<B>& b) noexcept;
    
    template <std::size_t N>
    class StaticString
    {
        template<std::size_t A, std::size_t B>
        friend constexpr auto operator+ (const StaticString<A>& a, const StaticString<B>& b) noexcept;
    public:
        std::array<char, N> string;
    private:
        template<std::size_t ...I>
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

    template<std::size_t N>
    StaticString(const char (&string)[N]) -> StaticString<N - 1>;

    template<std::size_t N>
    constexpr auto string(const char (&str)[N]) noexcept
    {
        return StaticString<N - 1>(str);
    }

    template<std::size_t Lhs, std::size_t Rhs>
    constexpr auto operator+ (const StaticString<Lhs>& lhs, const StaticString<Rhs>& rhs) noexcept
    {
        return StaticString<Lhs + Rhs>(lib::concat(lhs.string, rhs.string));
    }

    template<std::size_t Lhs, std::size_t Rhs>
    constexpr auto operator == (const StaticString<Lhs>& lhs, const StaticString<Rhs>& rhs) noexcept
    {
        return static_cast<std::string_view>(lhs) == static_cast<std::string_view>(rhs);
    }

    template<std::size_t Lhs, std::size_t Rhs>
    constexpr auto operator != (const StaticString<Lhs>& lhs, const StaticString<Rhs>& rhs) noexcept
    {
        return static_cast<std::string_view>(lhs) != static_cast<std::string_view>(rhs);
    }

    template<std::size_t Lhs, std::size_t Rhs>
    constexpr auto operator < (const StaticString<Lhs>& lhs, const StaticString<Rhs>& rhs) noexcept
    {
        return static_cast<std::string_view>(lhs) < static_cast<std::string_view>(rhs);
    }

    namespace details {
        constexpr int numDigits(int x) noexcept
        {
            if(x < 0) {
                return 1 + numDigits(-x);
            } else {
                if(x < 10) {
                    return 1;
                } else {
                    return 1 + numDigits(x / 10);
                }
            }
        }
    }

    template<int Value>
    constexpr auto toString() noexcept
    {
        auto value = Value;
        std::array<char, details::numDigits(Value)> result {};
        std::size_t index = result.size() - 1;
        if(value < 0) {
            result[0] = '-';
            value = -value;
        }
        do {
            result[index--] = '0' + (value % 10);
            value /= 10;
        } while(value);
        return StaticString(result);
    }

}
