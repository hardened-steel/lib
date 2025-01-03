#pragma once
#include <limits>
#include <cstdint>
#include <lib/array.hpp>

namespace lib::literal {

    namespace details {

        using MaxType = unsigned long long;

        template<class T>
        struct BaseType {
            using Type = T;
        };

        template<MaxType Value>
        auto select() noexcept
        {
            if constexpr(Value <= std::numeric_limits<std::uint16_t>::max()) {
                return BaseType<std::uint16_t>{};
            } else if constexpr(Value <= std::numeric_limits<std::uint32_t>::max()) {
                return BaseType<std::uint32_t>{};
            } else if constexpr(Value <= std::numeric_limits<std::uint64_t>::max()) {
                return BaseType<std::uint64_t>{};
            } else {
                static_assert(Value <= std::numeric_limits<std::uint64_t>::max());
            }
        }

        template<std::uint8_t Base, char Char>
        struct Digit;

        template<char Char>
        struct Digit<2, Char>
        {
            constexpr static inline std::uint8_t value = Char - '0';
        };

        template<char Char>
        struct Digit<8, Char>
        {
            constexpr static inline std::uint8_t value = Char - '0';
        };

        template<char Char>
        struct Digit<10, Char>
        {
            constexpr static inline std::uint8_t value = Char - '0';
        };

        template<char Char>
        struct Digit<16, Char>
        {
            constexpr static inline std::uint8_t value = (Char >= '0' && Char <= '9') ? Char - '0' : ((Char >= 'A' && Char <= 'F') ? (Char - 'A') + 10 : (Char - 'a') + 10);
        };

        template<std::uint8_t Base, char ...Chars>
        struct Number
        {
            constexpr static MaxType value() noexcept
            {
                MaxType result = 0;
                constexpr std::array digits {Digit<Base, Chars>::value...};
                for(auto digit: digits) {
                    result *= Base;
                    result += digit;
                }
                return result;
            }
        };
    }

    template<char ...Chars>
    struct Parser
    {
        constexpr static inline auto value = details::Number<10, Chars...>::value();
        using Type = typename decltype(details::select<value>())::Type;
    };

    template<char ...Chars>
    struct Parser<'0', 'x', Chars...>
    {
        constexpr static inline auto value = details::Number<16, Chars...>::value();
        using Type = typename decltype(details::select<value>())::Type;
    };

    template<char ...Chars>
    struct Parser<'0', 'X', Chars...>
    {
        constexpr static inline auto value = details::Number<16, Chars...>::value();
        using Type = typename decltype(details::select<value>())::Type;
    };

    template<char ...Chars>
    struct Parser<'0', 'b', Chars...>
    {
        constexpr static inline auto value = details::Number<2, Chars...>::value();
        using Type = typename decltype(details::select<value>())::Type;
    };

    template<char ...Chars>
    struct Parser<'0', 'B', Chars...>
    {
        constexpr static inline auto value = details::Number<2, Chars...>::value();
        using Type = typename decltype(details::select<value>())::Type;
    };

    template<char ...Chars>
    struct Parser<'0', Chars...>
    {
        constexpr static inline auto value = details::Number<8, Chars...>::value();
        using Type = typename decltype(details::select<value>())::Type;
    };

    template<char ...Chars>
    constexpr auto parse() noexcept
    {
        using Parser = Parser<Chars...>;
        return static_cast<typename Parser::Type>(Parser::value);
    }
}
